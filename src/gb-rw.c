#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

#include "gb-rw.h"
#include "usart.h"
#include "buffer.h"
#include "repeat.h"

/* STM32F411-Nucleo at 96 MHz */
const struct rcc_clock_scale rcc_hse_8mhz_3v3_96mhz = {
	.pllm = 8,
	.plln = 384,
	.pllp = 4,
	.pllq = 8,
	.pllr = 0,
	.hpre = RCC_CFGR_HPRE_DIV_NONE,
	.ppre1 = RCC_CFGR_PPRE_DIV_2,
	.ppre2 = RCC_CFGR_PPRE_DIV_NONE,
	.power_save = 1,
	.flash_config = FLASH_ACR_ICEN | FLASH_ACR_DCEN |
		FLASH_ACR_LATENCY_3WS,
	.ahb_frequency  = 96000000,
	.apb1_frequency = 48000000,
	.apb2_frequency = 96000000,
};

static void
clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3_96mhz);
	//rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	/* Enable GPIOA clock for LED, USARTs & bus signals. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable GPIOB for bus data. */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* Enable GPIOC for bus address. */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);

	/* Enable DMA1 clock */
	rcc_periph_clock_enable(RCC_DMA1);
}

static void
gpio_data_setup_output(void)
{
	/* Setup GPIO pins GPIO{0..7} on GPIO port B for bus data. */
	gpio_mode_setup(GPIOP_DATA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO0_7);
}

static void
gpio_data_setup_input(void)
{
	/* Setup GPIO pins GPIO{0..7} on GPIO port B for bus data. */
	gpio_mode_setup(GPIOP_DATA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0_7);
}

static void
gpio_setup(void)
{
	/* Setup GPIO pin GPIO5 on GPIO port A for LED. */
	gpio_mode_setup(GPIOP_LED, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPION_LED);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOP_USART, GPIO_MODE_AF, GPIO_PUPD_NONE, GPION_USART_TX | GPION_USART_RX);

	/* Setup USART2 TX/RX pin as alternate function. */
	gpio_set_af(GPIOP_USART, GPIO_AF7, GPION_USART_TX | GPION_USART_RX);

	/* Setup GPIO pins GPIO{0..15} on GPIO port C for bus address. */
	gpio_mode_setup(GPIOP_ADDR, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0_7 | GPIO8_15);

	/* Setup GPIO pins GPIO{0..7} on GPIO port B for bus data. */
	gpio_data_setup_input();

	/* Set GPIO pins GPIO{6,7,8,9,10} on GPIO port A for bus signals. */
	gpio_mode_setup(GPIOP_SIGNAL, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
			GPION_WR | GPION_RD | GPION_CS | GPION_RESET | GPION_CLK);

	gpio_set(GPIOP_SIGNAL, GPION_WR);
	gpio_set(GPIOP_SIGNAL, GPION_RD);
	gpio_set(GPIOP_SIGNAL, GPION_CS);
	gpio_set(GPIOP_SIGNAL, GPION_RESET);
}

enum dma_state {READY, BUSY, DONE};
volatile enum dma_state dma_send_state = READY;

void
dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
	        // Clear Transfer Complete Interrupt Flag
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
		dma_send_state = DONE;
	}

	dma_disable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
	usart_disable_tx_dma(USART2);
	dma_disable_stream(DMA1, DMA_STREAM6);
}

volatile int dma_recvd = 0;

void
dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
	        // Clear Transfer Complete Interrupt Flag
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		dma_recvd = 1;
	}

	dma_disable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
	usart_disable_rx_dma(USART2);
	dma_disable_stream(DMA1, DMA_STREAM5);
}

static inline void
delay_nop(unsigned int t)
{
	unsigned int i;
	for (i = 0; i < t; i++) { /* Wait a bit. */
		__asm__("nop");
	}
}

static void
usart_irq_setup(void)
{
	nvic_set_priority(NVIC_USART2_IRQ, 0x10);
	nvic_enable_irq(NVIC_USART2_IRQ);
	usart_enable_rx_interrupt(USART2);
}

struct circular_buf op_buf;

enum state {CMD, ARG0, ARG1, ARG2, ARG3};
enum state state;

enum cmd {READ, WRITE, WRITE_RAW, WRITE_FLASH, EREASE};

struct op {
	enum cmd cmd;
	union {
		uint16_t addr_start;
		struct {
			uint8_t addr_start_lo;
			uint8_t addr_start_hi;
		};
	};
	union {
		uint16_t addr_end;
		struct {
			uint8_t addr_end_lo;
			uint8_t addr_end_hi;
		};
		uint8_t data;
	};
};

static void
update_state(uint8_t b)
{
	static enum cmd cmd;
	static uint8_t arg[4];
	static struct op op;

	switch (state) {
	case CMD:
		cmd = b;
		state = ARG0;
		break;
	case ARG0:
		arg[0] = b;
		state = ARG1;
		break;
	case ARG1:
		arg[1] = b;
		state = ARG2;
		break;
	case ARG2:
		arg[2] = b;
		state = ARG3;
		break;
	case ARG3:
		arg[3] = b;

		op.cmd = cmd;
		op.addr_start_lo = arg[0];
		op.addr_start_hi = arg[1];
		//op.data = arg[2];
		op.addr_end_lo   = arg[2];
		op.addr_end_hi   = arg[3];

		switch (op.cmd) {
		case WRITE_RAW:
		case WRITE_FLASH:
			// TODO: Prepare to receive DMA or NACK
		case READ:
		case WRITE:
		case EREASE:
			buf_push(&op_buf, &op);
			// TODO: ACK
			break;
		default:
			break;
		}

		state = CMD;
		break;
	default:
		break;
	}
}

void
usart2_isr(void)
{
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(USART2) & USART_SR_RXNE) != 0)) {
		update_state(usart_recv(USART2));
	}
}

uint8_t read_buf[0x4000];

static inline uint8_t
bus_read_byte(uint16_t addr)
{
	static uint8_t data;

	gpio_clear(GPIOP_SIGNAL, GPION_RD);
	// Set address
	gpio_port_write(GPIOP_ADDR, addr & 0xffff);
	// wait some nanoseconds
	delay_nop(50);
	gpio_clear(GPIOP_SIGNAL, GPION_CS);
	// wait ~200ns
	//REP(2,0,asm("NOP"););
	delay_nop(50000);
	// read data
	data = gpio_port_read(GPIOP_DATA) & 0xff;
	//data = gpio_get(GPIOP_DATA, GPIO0);

	gpio_set(GPIOP_SIGNAL, GPION_CS);
	delay_nop(500);

	return data;
}

static void
bus_read_bytes(uint16_t addr_start, uint16_t addr_end, uint8_t *buf)
{
	int i;
	for (i = 0; i < (addr_end - addr_start); i++) {
		buf[i] = bus_read_byte((uint16_t) addr_start + i);
	}
}

static inline void
bus_write_byte(uint16_t addr, uint8_t data)
{

}

int
main(void)
{
	struct op op;
	int i;

	buf_init(&op_buf, sizeof(struct op));

	clock_setup();
	gpio_setup();
	//usart_setup(115200);
	//usart_setup(1152000);
	usart_setup(1000000);
	usart_send_dma_setup();
	usart_recv_dma_setup();

	for (i = 0; i < 10; i++) {
		usart_recv(USART2); // Clear initial garbage
	}
	usart_irq_setup();
	usart_send_srt_blocking("\nHELLO\n");

	while (1) {
		while (buf_empty(&op_buf));
		buf_pop(&op_buf, &op);
		gpio_toggle(GPIOP_LED, GPION_LED);

		switch (op.cmd) {
		case READ:
			//bus_read_bytes(0x00, 0x10, read_buf);
			//read_buf[0] = 'H';
			//read_buf[1] = 'O';
			//read_buf[2] = 'L';
			//read_buf[3] = 'A';
			bus_read_bytes(op.addr_start, op.addr_end, read_buf);
			while (dma_send_state == BUSY);
			dma_send_state = BUSY;
			usart_send_dma(read_buf, op.addr_end - op.addr_start);
			break;
		case WRITE:
			break;
		case WRITE_RAW:
			break;
		case WRITE_FLASH:
			break;
		case EREASE:
			break;
		default:
			break;
		}
	}

	return 0;
}
