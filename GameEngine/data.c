#include "data.h"

static Buffer* buffer_pool;

Buffer* buffer_alloc() {
	if (!buffer_pool) {
		buffer_pool = malloc(MAX_BUFF_COUNT * sizeof(Buffer));
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

void buffer_free(Buffer* buffer) {
	buffer->len = -1;
}