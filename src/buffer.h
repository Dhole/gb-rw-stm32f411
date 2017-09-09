#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <string.h>

#define BUF_LEN 1024

struct circular_buf {
	uint8_t buf[BUF_LEN];
	volatile uint32_t head;
	volatile uint32_t tail;
	uint32_t len;
	uint32_t elem_size;
};

static inline void
buf_push(struct circular_buf *buf, void *elem)
{
	//buf->buf[buf->head] = b;
	memcpy(&buf[buf->head * buf->elem_size], elem, buf->elem_size);
	buf->head = (buf->head + 1) % buf->len;
}

static inline void
buf_pop(struct circular_buf *buf, void *elem)
{
	//uint8_t b = buf->buf[buf->tail];
	memcpy(elem, &buf[buf->tail * buf->elem_size], buf->elem_size);
	buf->tail = (buf->tail + 1) % buf->len;
}

static inline void
buf_clear(struct circular_buf *buf)
{
	buf->head = 0;
	buf->tail = 0;
}

static inline void
buf_init(struct circular_buf *buf, int elem_size)
{
	buf_clear(buf);
	buf->elem_size = elem_size;
	buf->len = BUF_LEN / elem_size;
}

static inline int
buf_empty(struct circular_buf *buf)
{
	return (buf->tail == buf->head);
}

#endif
