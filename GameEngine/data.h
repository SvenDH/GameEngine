#pragma once
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF_SIZE 528392
#define MAX_BUFF_COUNT 16

typedef struct Buffer {
	char data[MAX_BUFF_SIZE];
	int len;
} Buffer;

Buffer* buffer_alloc();
void buffer_free(Buffer* buffer);