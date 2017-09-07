#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/timer.h>
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
	gpio_mode_setup(GPIOP_DATA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0_7);

	/* Set GPIO pins GPIO{6,7,8,9,10} on GPIO port A for bus signals. */
	gpio_mode_setup(GPIOP_SIGNAL, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
			GPION_WR | GPION_RD | GPION_CS | GPION_RESET | GPION_CLK);

	gpio_set(GPIOP_SIGNAL, GPION_WR);
	gpio_set(GPIOP_SIGNAL, GPION_RD);
	gpio_set(GPIOP_SIGNAL, GPION_CS);
	gpio_set(GPIOP_SIGNAL, GPION_RESET);
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

volatile int dma_sent = 0;

void
dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
	        // Clear Transfer Complete Interrupt Flag
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
		dma_sent = 1;
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

//static void
//gblink_sniff_gpio_setup(void)
//{
//	// PA0 -> SCK
//	gpio_mode_setup(GPIOP_SCK, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SCK);
//	// PC0 -> SIN
//	gpio_mode_setup(GPIOP_SIN, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SIN);
//	// PC1 -> SOUT
//	gpio_mode_setup(GPIOP_SOUT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SOUT);
//	// PC2 -> SD
//	gpio_mode_setup(GPIOP_SD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SD);
//
//	nvic_set_priority(NVIC_EXTI0_IRQ, 0);
//	nvic_enable_irq(NVIC_EXTI0_IRQ);
//
//	exti_select_source(EXTI0, GPIOP_SCK);
//	//exti_set_trigger(EXTI0, EXTI_TRIGGER_FALLING);
//	exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
//	exti_enable_request(EXTI0);
//}

//static void
//gblink_slave_gpio_setup(void)
//{
//	// PA0 -> SCK
//	gpio_mode_setup(GPIOP_SCK, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SCK);
//	// PC0 -> SIN
//	gpio_mode_setup(GPIOP_SIN, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPION_SIN);
//	gpio_set_output_options(GPIOP_SIN, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPION_SIN);
//	gpio_clear(GPIOP_SIN, GPION_SIN);
//	// PC1 -> SOUT
//	gpio_mode_setup(GPIOP_SOUT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPION_SOUT);
//	// PC2 -> SD
//	//gpio_mode_setup(GPIOP_SD, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPION_SD);
//	//gpio_mode_setup(GPIOP_SD, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPION_SD);
//
//	//gpio_set(GPIOP_SD, GPION_SD);
//
//	nvic_set_priority(NVIC_EXTI0_IRQ, 0);
//	nvic_enable_irq(NVIC_EXTI0_IRQ);
//	nvic_set_priority(NVIC_USART2_IRQ, 1);
//	nvic_enable_irq(NVIC_USART2_IRQ);
//
//	exti_select_source(EXTI0, GPIOP_SCK);
//	exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH);
//	exti_enable_request(EXTI0);
//
//	usart_enable_rx_interrupt(USART2);
//}

struct circular_buf recv_buf;

void
usart2_isr(void)
{
	uint8_t empty;
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(USART2) & USART_SR_RXNE) != 0)) {
		//empty = buf_empty(&recv_buf);

		buf_push(&recv_buf, usart_recv(USART2));

		//if (empty && gb_bit == 0 ) {
		//	gb_sin = buf_pop(&recv_buf);
		//}
	}
}

static void
tim_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);

	/* Enable TIM2 interrupt. */
	nvic_enable_irq(NVIC_TIM2_IRQ);

	/* Reset TIM2 peripheral to defaults. */
	rcc_periph_reset_pulse(RST_TIM2);

	/* Timer global mode:
	 * - No divider
	 * - Alignment edge
	 * - Direction up
	 * (These are actually default values after reset above, so this call
	 * is strictly unnecessary, but demos the api for alternative settings)
	 */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	/*
	 * Please take note that the clock source for STM32 timers
	 * might not be the raw APB1/APB2 clocks.  In various conditions they
	 * are doubled.  See the Reference Manual for full details!
	 * In our case, TIM2 on APB1 is running at double frequency, so this
	 * sets the prescaler to have the timer run at 5kHz
	 */
	/* Set the prescaler to run at 1MHz */
	timer_set_prescaler(TIM2, ((rcc_apb1_frequency * 2) / 1000000) - 1);

	/* Set the initual output compare value for OC1. */
	timer_set_oc_value(TIM2, TIM_OC1, 0);

	/* Disable preload. */
	timer_disable_preload(TIM2);
	timer_continuous_mode(TIM2);

	/* count full range, as we'll update compare value continuously */
	//timer_set_period(TIM2, 8 - 1);
	timer_set_period(TIM2, 4 - 1);
}

static inline void
tim_start(void)
{
	/* Counter enable. */
	timer_enable_counter(TIM2);

	/* Enable Channel 1 compare interrupt to recalculate compare values */
	timer_enable_irq(TIM2, TIM_DIER_CC1IE);
}

static inline void
tim_stop(void)
{
	timer_disable_counter(TIM2);
	timer_disable_irq(TIM2, TIM_DIER_CC1IE);
}

uint32_t addr;
uint8_t data;
uint8_t buf[0x4000];
volatile uint8_t done;

static inline uint8_t
bus_read(uint16_t addr)
{
	uint8_t data;

	gpio_clear(GPIOP_SIGNAL, GPION_RD);
	// Set address
	gpio_port_write(GPIOP_ADDR, addr & 0xffff);
	// wait some nanoseconds
	//delay_nop(5);
	gpio_clear(GPIOP_SIGNAL, GPION_CS);
	// wait 200ns
	REP(2,0,asm("NOP"));
	// read data
	data = gpio_port_read(GPIOP_DATA) & 0xff;
	//data = gpio_get(GPIOP_DATA, GPIO0);

	gpio_set(GPIOP_SIGNAL, GPION_CS);

	return data;
}

static inline uint8_t
bus_write(uint16_t addr, uint8_t data)
{

}

void
tim2_isr(void)
{
	if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {

		/* Toggle LED to indicate compare event. */
		//gpio_toggle(GPIOP_LED, GPION_LED);

		if (addr >= 0x4000) {
		//if (addr >= 400) {
			tim_stop();
			done = 1;
		} else {
			gpio_clear(GPIOP_SIGNAL, GPION_RD);
			// Set address
			gpio_port_write(GPIOP_ADDR, addr & 0xffff);
			// wait some nanoseconds
			//delay_nop(5);
			gpio_clear(GPIOP_SIGNAL, GPION_CS);
			// wait 500ns
			delay_nop(5);
			// read data
			data = gpio_port_read(GPIOP_DATA) & 0xff;
			//data = gpio_get(GPIOP_DATA, GPIO0);

			gpio_set(GPIOP_SIGNAL, GPION_CS);

			//usart_send_blocking(USART2, data);
			buf[addr] = data;
			addr++;
		}
		//if (addr > 0xffff) {
		//if (addr > 400) {

		/* Clear compare interrupt flag. */
		timer_clear_flag(TIM2, TIM_SR_CC1IF);
	}
}

int
main(void)
{
	uint8_t opt;
	uint8_t cont;

	buf_clear(&recv_buf);

	clock_setup();
	gpio_setup();
	//usart_setup(115200);
	//usart_setup(1152000);
	usart_setup(1000000);
	usart_send_dma_setup();
	usart_recv_dma_setup();

	usart_recv(USART2); // Clear initial garbage
	usart_send_srt_blocking("\nHELLO\n");

	done = 0;
	addr = 0;

	//gpio_toggle(GPIOP_LED, GPION_LED); /* LED on/off */

	//while (1) {
	//	opt = usart_recv_blocking(USART2);
	//	switch (opt) {
	//	case MODE_SNIFF:
	//		mode = MODE_SNIFF;
	//		gblink_sniff_gpio_setup();
	//		while (1);
	//		break;
	//	case SLAVE_PRINTER:
	//		mode = MODE_SLAVE;
	//		slave_mode = SLAVE_PRINTER;
	//		printer_reset_state();
	//		gblink_slave_gpio_setup();
	//		while (1);
	//		break;
	//	case MODE_SLAVE:
	//		mode = MODE_SLAVE;
	//		gblink_slave_gpio_setup();
	//		while (1);
	//		break;
	//	default:
	//		break;
	//	}
	//}

	tim_setup();

	while (1) {
		opt = usart_recv_blocking(USART2);

		switch (opt) {
		case 's':
			tim_start();
			break;
		default:
			cont = 1;
			break;
		}
		if (cont) {
			cont = 0;
			continue;
		}
		while (!done);
		usart_send_dma(buf, 0x4000);
	}

	while (1);

	return 0;
}
