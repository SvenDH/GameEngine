#include "data.h"

#define INITIAL_QUEUE_SIZE 4096

//TODO: mutexes and conditionals
void queue_init(queue_t* q, size_t element_size) {
	bip_init(q, malloc(INITIAL_QUEUE_SIZE * element_size), INITIAL_QUEUE_SIZE * element_size);
	q->element_size = element_size;
}

int queue_put(queue_t* q, void* data) {
	size_t len = q->element_size;
	char* ptr = bip_alloc(q, len);
	if (ptr) {
		memcpy(ptr, data, len);
		return bip_write(q, len);
	}
	return 0;
}

int queue_get(queue_t* q, void* data) {
	size_t len = q->element_size;
	char* ptr = bip_peek(q, len);
	if (ptr) {
		memcpy(data, ptr, len);
		return bip_read(q, len);
	}
	return 0;
}

/*
int serialize_bytes(buffer_t* buf, uint8_t* data, int32_t len) {
	char* ptr = bip_alloc(buf, len);
	if (ptr) {
		memcpy(ptr, data, len);
		return bip_write(buf, len);
	}
	return 0;
}

int deserialize_bytes(buffer_t* buf, uint8_t* data, int32_t len) {
	char* ptr = bip_peek(buf, len);
	if (ptr) {
		memcpy(data, ptr, len);
		return bip_read(buf, len);
	}
	return 0;
}

//TODO: check for min/max values
int serialize_int(buffer_t* buf, int32_t* integer) {
	int32_t sint = htonl(*integer);
	return serialize_bytes(buf, &sint, sizeof(int32_t));
}

int deserialize_int(buffer_t* buf, int32_t* integer) {
	int r = deserialize_bytes(buf, integer, 4);
	*integer = ntohl(*integer);
	return r;
}

//TODO: check if string fits
int serialize_string(buffer_t* buf, uint8_t* data, int32_t* len) {
	//assert(buf->max_ - buf->size_ >= 4 + *len);
	int r = serialize_int(buf, len);
	r += serialize_bytes(buf, data, *len);
	return 4 + *len;
}

int deserialize_string(buffer_t* buf, uint8_t* data, int32_t* len) {
	int r = deserialize_int(buf, len);
	r += deserialize_bytes(buf, data, *len);
	return 4 + *len;
}
*/