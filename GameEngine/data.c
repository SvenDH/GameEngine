#include "data.h"

static PoolBuffer* buffer_pool;

PoolBuffer* buffer_alloc() {
	if (!buffer_pool) {
		buffer_pool = malloc(MAX_BUFF_COUNT * sizeof(PoolBuffer));
		for (int i = 0; i < MAX_BUFF_COUNT; i++) buffer_pool[i].len = -1;
	}
	for (int i = 0; i < MAX_BUFF_COUNT; i++) {
		if (buffer_pool[i].len == -1) {
			buffer_pool[i].len = 0;
			return &buffer_pool[i];
		}
	}
	return NULL;
}

void buffer_free(PoolBuffer* buffer) {
	buffer->len = -1;
}

RingBuffer* ringbuffer_init(size_t size, void* array) {
	RingBuffer* rb = malloc(sizeof(RingBuffer));
	rb->max_ = size;
	rb->array_ = array;
	rb->head_ = 0;
	rb->tail_ = 0;
	return rb;
}