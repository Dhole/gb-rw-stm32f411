#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "usart.h"

void
usart_setup(uint32_t baud)
{
	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, baud);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

/* USART2_TX uses DMA controller 1 Stream 6 Channel 4. */
void
usart_send_dma_setup(void)
{
	dma_stream_reset(DMA1, DMA_STREAM6);

	nvic_set_priority(NVIC_DMA1_STREAM6_IRQ, 1);
	nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);

	dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t) &USART2_DR);
	dma_set_transfer_mode(DMA1, DMA_STREAM6, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

	dma_set_peripheral_size(DMA1, DMA_STREAM6, DMA_SxCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_STREAM6, DMA_SxCR_MSIZE_8BIT);

	dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_HIGH);

	dma_disable_peripheral_increment_mode(DMA1, DMA_STREAM6);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);

	dma_disable_transfer_error_interrupt(DMA1, DMA_STREAM6);
	dma_disable_half_transfer_interrupt(DMA1, DMA_STREAM6);
	dma_disable_direct_mode_error_interrupt(DMA1, DMA_STREAM6);
	dma_disable_fifo_error_interrupt(DMA1, DMA_STREAM6);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
}

void
usart_send_dma(uint8_t *data, int size)
{
	dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t) data);
	dma_set_number_of_data(DMA1, DMA_STREAM6, size);

	dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_4);
	dma_enable_stream(DMA1, DMA_STREAM6);
        usart_enable_tx_dma(USART2);
}

/* USART2_RX uses DMA controller 1 Stream 7 Channel 6. */
/* USART2_RX uses DMA controller 1 Stream 5 Channel 4. */
void
usart_recv_dma_setup(void)
{
    dma_stream_reset(DMA1, DMA_STREAM5);

    nvic_set_priority(NVIC_DMA1_STREAM5_IRQ, 1);
    nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);

    dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) &USART2_DR);
    dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);

    dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);

    dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_VERY_HIGH);

    dma_disable_peripheral_increment_mode(DMA1, DMA_STREAM5);
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);

    dma_disable_transfer_error_interrupt(DMA1, DMA_STREAM5);
    dma_disable_half_transfer_interrupt(DMA1, DMA_STREAM5);
    dma_disable_direct_mode_error_interrupt(DMA1, DMA_STREAM5);
    dma_disable_fifo_error_interrupt(DMA1, DMA_STREAM5);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
}

void
usart_recv_dma(void *data, const int size)
{
    dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) data);
    dma_set_number_of_data(DMA1, DMA_STREAM5, size);

    dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_4);
    dma_enable_stream(DMA1, DMA_STREAM5);
    usart_enable_rx_dma(USART2);
}

void
usart_send_srt_blocking(const char *in)
{
	int i;
	for (i = 0; in[i] != '\0'; i++) {
		usart_send_blocking(USART2, in[i]);
	}
}

void
usart_send_bytes_blocking(const uint8_t* in, int n)
{
	int i;
	for(i = 0; i < n; i++) {
		usart_send_blocking(USART2, in[i]);
	}
}

void
usart_recv_bytes_blocking(uint8_t* out, int n)
{
	int i;
	for(i = 0; i < n; i++) {
		out[i] = usart_recv_blocking(USART2);
	}
}
