#ifndef USART_H
#define USART_H

void usart_setup(uint32_t baud);
void usart_send_dma_setup(void);
void usart_send_dma(uint8_t *data, int size);
void usart_recv_dma_setup(void);
void usart_recv_dma(void *data, const int size);
void usart_send_srt_blocking(const char *in);
void usart_send_bytes_blocking(const uint8_t* in, int n);
void usart_recv_bytes_blocking(uint8_t* out, int n);

#endif
