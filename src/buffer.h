#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <string.h>

#define BUF_LEN 0x4000

struct circular_buf {
	uint8_t buf[BUF_LEN];
	volatile size_t head;
	volatile size_t tail;
	size_t len;
	size_t elem_size;
};

static inline void
buf_push(struct circular_buf *cbuf, void *elem)
{
	memcpy(&cbuf->buf[cbuf->head * cbuf->elem_size], elem, cbuf->elem_size);
	cbuf->head = (cbuf->head + 1) % cbuf->len;
}

static inline void
buf_pop(struct circular_buf *cbuf, void *elem)
{
	//uint8_t b = buf->buf[buf->tail];
	memcpy(elem, &cbuf->buf[cbuf->tail * cbuf->elem_size], cbuf->elem_size);
	cbuf->tail = (cbuf->tail + 1) % cbuf->len;
}

static inline void
buf_clear(struct circular_buf *cbuf)
{
	cbuf->head = 0;
	cbuf->tail = 0;
}

static inline void
buf_init(struct circular_buf *cbuf, size_t elem_size)
{
	buf_clear(cbuf);
	cbuf->elem_size = elem_size;
	cbuf->len = (BUF_LEN / elem_size) - 1;
}

static inline int
buf_empty(struct circular_buf *cbuf)
{
	return (cbuf->tail == cbuf->head);
}

#endif
