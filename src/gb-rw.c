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
	gpio_mode_setup(GPIOP_DATA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0_7);
}

static void
gpio_data_setup_input(void)
{
	/* Setup GPIO pins GPIO{0..7} on GPIO port B for bus data. */
	gpio_mode_setup(GPIOP_DATA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0_7);
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
	gpio_mode_setup(GPIOP_ADDR, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0_7 | GPIO8_13);
	gpio_mode_setup(GPIOP_ADDRH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPION_ADDR14 | GPION_ADDR15);

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

#define READ_BUF_LEN 0x4000
uint8_t read_buf[2][READ_BUF_LEN];
uint8_t read_buf_slot;
volatile uint8_t read_buf_slot_busy;

#define WRITE_BUF_LEN 0x4000
uint8_t write_buf[2][WRITE_BUF_LEN];
uint8_t write_buf_slot;
volatile uint8_t write_buf_slot_busy;
volatile uint8_t write_buf_slot_ready;

enum dma_state {READY, BUSY, DONE};
volatile enum dma_state dma_send_state = READY;

void
dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
	        // Clear Transfer Complete Interrupt Flag
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
		dma_send_state = DONE;
		read_buf_slot_busy--;
	}

	dma_disable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
	usart_disable_tx_dma(USART2);
	dma_disable_stream(DMA1, DMA_STREAM6);
}

volatile enum dma_state dma_recv_state = READY;

void
dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
	        // Clear Transfer Complete Interrupt Flag
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		dma_recv_state = DONE;
		write_buf_slot_ready++;
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

enum cmd {READ, WRITE, WRITE_RAW, WRITE_FLASH, EREASE, RESET, PING};
enum cmd_reply {DMA_READY, DMA_NOT_READY, PONG};

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
	uint8_t buf_slot;
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
			if (((op.addr_start > op.addr_end) ||
			     (op.addr_end - op.addr_start) > WRITE_BUF_LEN)
			    || (write_buf_slot_busy >= 2)
			    || (dma_recv_state == BUSY)
			   ) {
				usart_send_blocking(USART2, DMA_NOT_READY);
				break;
			}
			write_buf_slot_busy++;
			dma_recv_state = BUSY;
			usart_recv_dma(write_buf[write_buf_slot], op.addr_end - op.addr_start);
			usart_send_blocking(USART2, DMA_READY);
			op.buf_slot = write_buf_slot;
			buf_push(&op_buf, &op);
			write_buf_slot = (write_buf_slot + 1) % 2;
			break;
		case WRITE:
			buf_push(&op_buf, &op);
			break;
		case EREASE:
			break;
		case READ:
			if ((op.addr_start > op.addr_end) ||
			    (op.addr_end - op.addr_start) > READ_BUF_LEN) {
				break;
			}
			buf_push(&op_buf, &op);
			break;
		case RESET:
			buf_push(&op_buf, &op);
			break;
		case PING:
			buf_push(&op_buf, &op);
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

static inline void
set_addr(uint16_t addr)
{
	gpio_port_write(GPIOP_ADDR, addr);
	(addr & 0x4000) ? gpio_set(GPIOP_ADDRH, GPION_ADDR14) : gpio_clear(GPIOP_ADDRH, GPION_ADDR14);
	(addr & 0x8000) ? gpio_set(GPIOP_ADDRH, GPION_ADDR15) : gpio_clear(GPIOP_ADDRH, GPION_ADDR15);
}

static inline void
set_data(uint8_t data)
{
	gpio_port_write(GPIOP_DATA, data);
}

static inline uint8_t
get_data(void)
{
	return gpio_port_read(GPIOP_DATA) & 0xff;
}

static inline void
set_cs(void)
{
	gpio_clear(GPIOP_SIGNAL, GPION_CS);
}

static inline void
unset_cs(void)
{
	gpio_set(GPIOP_SIGNAL, GPION_CS);
}

static inline void
set_rd(void)
{
	gpio_clear(GPIOP_SIGNAL, GPION_RD);
}

static inline void
unset_rd(void)
{
	gpio_set(GPIOP_SIGNAL, GPION_RD);
}

static inline void
set_wr(void)
{
	gpio_clear(GPIOP_SIGNAL, GPION_WR);
}

static inline void
unset_wr(void)
{
	gpio_set(GPIOP_SIGNAL, GPION_WR);
}

static inline uint8_t
bus_read_byte(uint16_t addr)
{
	static uint8_t data;

	// Set address
	set_addr(addr);
	set_cs();
	// wait some nanoseconds
	NOP_REP(0,5);
	set_rd();
	// wait ~200ns
	NOP_REP(2,0);
	// read data
	data = get_data();
	//data = gpio_get(GPIOP_DATA, GPIO0);

	unset_rd();
	unset_cs();
	NOP_REP(1,0);

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
	// Set address
	set_addr(addr);
	//unset_rd();
	// wait some nanoseconds
	NOP_REP(0,5);

	set_cs();
	NOP_REP(0,5);
	set_wr();

	// set data
	gpio_data_setup_output();
	set_data(data);
	// wait ~200ns
	NOP_REP(2,0);

	unset_wr();
	NOP_REP(0,5);
	gpio_data_setup_input();
	unset_cs();
	//set_rd();
	NOP_REP(1,5);
}

static inline void
bus_write_bytes(uint16_t addr_start, uint16_t addr_end, uint8_t *buf)
{
	int i;

	for (i = 0; i < (addr_end - addr_start); i++) {
		bus_write_byte((uint16_t) addr_start + i, buf[i]);
	}
}

static inline void
bus_write_flash_byte(uint16_t addr, uint8_t data)
{
	bus_write_byte(0x0AAA, 0xA9);
	bus_write_byte(0x0555, 0x56);
	bus_write_byte(0x0AAA, 0xA0);
	bus_write_byte(addr, data);
	delay_nop(100);
}

static inline void
bus_write_flash_bytes(uint16_t addr_start, uint16_t addr_end, uint8_t *buf)
{
	int i;

	// For some reason, the first byte may not be written, so we force the
	// first write to do nothing.
	bus_write_flash_byte((uint16_t) addr_start, 0xFF);
	for (i = 0; i < (addr_end - addr_start); i++) {
		bus_write_flash_byte((uint16_t) addr_start + i, buf[i]);
	}
}

int
main(void)
{
	struct op op;
	int i;

	buf_init(&op_buf, sizeof(struct op));

	read_buf_slot = 0;
	read_buf_slot_busy = 0;
	write_buf_slot = 0;
	write_buf_slot_busy = 0;
	write_buf_slot_ready = 0;

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

	//gpio_port_write(GPIOP_ADDR, 0xffff);
	//gpio_set(GPIOC, GPIO15);
	//gpio_set(GPIOC, GPIO14);
	//gpio_set(GPIOC, GPIO13);
	//gpio_set(GPIOC, GPIO12);
	//gpio_set(GPIOP_ADDR, GPIO13);
	//gpio_set(GPIOP_ADDRH, GPION_ADDR14);
	//gpio_set(GPIOP_ADDRH, GPION_ADDR15);
	//while (1);

	while (1) {
		while (buf_empty(&op_buf));
		gpio_toggle(GPIOP_LED, GPION_LED);
		buf_pop(&op_buf, &op);

		switch (op.cmd) {
		case READ:
			while (read_buf_slot_busy >= 2);
			read_buf_slot_busy++;
			bus_read_bytes(op.addr_start, op.addr_end, read_buf[read_buf_slot]);
			while (dma_send_state == BUSY);
			dma_send_state = BUSY;
			usart_send_dma(read_buf[read_buf_slot], op.addr_end - op.addr_start);
			read_buf_slot = (read_buf_slot + 1) % 2;
			break;
		case WRITE:
			bus_write_byte(op.addr_start, op.data);
			break;
		case WRITE_RAW:
			while (write_buf_slot_ready == 0);
			bus_write_bytes(op.addr_start, op.addr_end, write_buf[op.buf_slot]);
			write_buf_slot_ready--;
			write_buf_slot_busy--;
			break;
		case WRITE_FLASH:
			while (write_buf_slot_ready == 0);
			bus_write_flash_bytes(op.addr_start, op.addr_end, write_buf[op.buf_slot]);
			write_buf_slot_ready--;
			write_buf_slot_busy--;
			break;
		case EREASE:
			break;
		case RESET:
			gpio_clear(GPIOP_SIGNAL, GPION_RESET);
			NOP_REP(1,0);
			gpio_set(GPIOP_SIGNAL, GPION_RESET);
			break;
		case PING:
			usart_send_blocking(USART2, PONG);
			break;
		default:
			break;
		}
	}

	return 0;
}
