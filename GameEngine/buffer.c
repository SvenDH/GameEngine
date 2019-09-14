#include "memory.h"

void bip_init(buffer_t* buf, void* data, size_t size) {
	ALLOCATOR_INIT(buf, data, size);
	bip_clear(buf);
}

void* bip_alloc(buffer_t* buf, size_t size) {
	if (size > 0 && size < buf->cap) {
		size_t read = buf->read; //Atomic
		size_t write = buf->write; //Atomic
		if (write >= read) {
			size_t available = buf->cap - write - ((read > 0) ? 0 : 1);
			if (size <= available)
				return &buf->data[write];
			else {
				if (size < read)
					return buf->data;
				else if ((write == read) && (size == read)) {
					buf->read = 0; //Atomic
					buf->write = 0; //Atomic
					return buf->data;
				}
			}
		}
		else if ((write + size) < read)
			return &buf->data[write];
	}
	return NULL;
}

int bip_write(buffer_t* buf, size_t size) {
	buf->used += size;
	if (size > 0 && size < buf->cap) {
		size_t read = buf->read; //Atomic
		size_t write = buf->write; //Atomic
		if (write >= read) {
			size_t available = buf->cap - write - ((read > 0) ? 0 : 1);
			if (size <= available) {
				if (size < (buf->cap - write)) {
					buf->write = write + size; //Atomic
					return 1;
				}
				else if (size == (buf->cap)) {
					buf->write = 0; //Atomic
					return 1;
				}
			}
			else if (size < read) {
				buf->end = write;
				buf->write = size; //Atomic
				return 1;
			}
		}
		else if ((write + size) < read) {
			buf->write = write + size; //Atomic
			return 1;
		}
	}
	else if (size == 0) return 1;
	buf->used -= size;
	return 0;
}

void* bip_peek(buffer_t* buf, size_t size) {
	if (size > 0 && size < buf->cap) {
		size_t read = buf->read; //Atomic
		size_t write = buf->write; //Atomic
		if ((write >= read) && ((read + size) <= write))
			return &buf->data[read];
		else if (read < buf->cap) {
			size_t wrap = buf->end; //Atomic
			if ((read + size) <= wrap)
				return &buf->data[read];
			else if (read == wrap && size <= write)
				return buf->data;
		}
	}
	return NULL;
}

int bip_read(buffer_t* buf, size_t size) {
	buf->used -= size;
	if (size > 0 && size < buf->cap) {
		size_t read = buf->read; //Atomic
		size_t write = buf->write; //Atomic
		if (read < write) {
			if (read + size <= write) {
				buf->read = read + size; //Atomic
				return 1;
			}
		}
		else if (read > write) {
			size_t wrap = buf->end; //Atomic
			if (read + size < wrap) { 
				buf->read = read + size; //Atomic
				return 1;
			}
			else if (read + size == wrap) {
				buf->end = buf->cap;
				buf->read = 0;
				return 1;
			}
			else if ((read == wrap) && (size <= write)) {
				buf->end = buf->cap;
				buf->read = size;
				return 1;
			}
		}
	}
	else if (size == 0) return 1;
	buf->used += size;
	return 0;
}

void bip_free(buffer_t* buf, void* ptr) {
	assert(0, "Not implemented");
}

void bip_clear(buffer_t* buf) {
	buf->write = 0;
	buf->read = 0;
	buf->end = buf->cap;
}