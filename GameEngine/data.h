#pragma once
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF_SIZE 528392
#define MAX_BUFF_COUNT 16

/* Data struct macros */

#define enqueue(b, r, t) \
	if (((b)->tail_ + 1) % (b)->max_ != (b)->head_) { \
		(b)->tail_ = ((b)->tail_ + 1) % (b)->max_; \
		((t*)(b)->array_)[(b)->tail_] = (r); \
		return 0; \
	} \
	else return 1

#define peek(b, r, t) \
	if ((b)->head_ != (b)->tail_) { \
		r = ((t*)(b)->array_)[(b)->head_]; \
	}

#define dequeue(b, r, t) \
	if ((b)->head_ != (b)->tail_) { \
		(b)->head_ = ((b)->head_ + 1) % (b)->max_; \
		r = ((t*)(b)->array_)[(b)->head_]; \
	}

#define push(b, r, t) \
	((t*)(b)->array_)[++(b)->top_] = r

#define pop(b, r, t) \
	r = ((t*)(b)->array_)[(b)->top_--]


typedef struct {
	char data[MAX_BUFF_SIZE];
	int len;
} PoolBuffer;

PoolBuffer* buffer_alloc();
void buffer_free(PoolBuffer* buffer);

typedef struct {
	char* array_;
	int head_;
	int tail_;
	int max_;
} RingBuffer;

RingBuffer* ringbuffer_init(size_t size, void* array);