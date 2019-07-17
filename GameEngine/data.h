#pragma once
#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define POOL_BLOCKS_INITIAL 1
#define MAX_BUFF_SIZE 528392
#define MAX_BUFF_COUNT 16

#define HASHMAP_INITIAL 256
#define HASHMAP_MAX_CHAIN_LENGTH 8

#define ALIGNMENT 64

#define WORD_BITS (8 * sizeof(unsigned int))

#define bitarray(_s_) (int *)calloc((_s_) / 8 + 1, sizeof(unsigned int));

static inline void setIndex(unsigned int * bitarray, size_t idx) {
	bitarray[idx / WORD_BITS] |= (1 << (idx % WORD_BITS));
}

typedef struct {
	struct free_node* next;
} free_node;

typedef struct {
	size_t elementsize_;
	size_t blocksize_;
	size_t used;
	int block_i;
	free_node *free_list;
	size_t blocksused;
	char **blocks;
} MemoryPool;

MemoryPool* pool_init(size_t elementsize, size_t blocksize);
void *pool_alloc(MemoryPool* p);
void pool_free(MemoryPool* p, void* ptr);
void pool_free_all(MemoryPool* p);

typedef struct {
	char* data_;
	int head_;
	int tail_;
	size_t max_;
	size_t size_;
	uv_mutex_t mutex;
} RingBuffer;

RingBuffer* ringbuffer_init(size_t size);
size_t buffer_write(RingBuffer* buf, char* data, size_t num);
size_t buffer_read(RingBuffer* buf, char* data, size_t num);
int serialize_bytes(RingBuffer* buf, uint8_t* data, int32_t len);
int deserialize_bytes(RingBuffer* buf, uint8_t* data, int32_t len);
int serialize_int(RingBuffer* buf, int32_t* integer);
int deserialize_int(RingBuffer* buf, int32_t* integer);
int serialize_string(RingBuffer* buf, uint8_t* data, int32_t* len);
int deserialize_string(RingBuffer* buf, uint8_t* data, int32_t* len);

typedef struct {
	char* key;
	size_t len;
	int in_use;
	void* data;
} hash_node;

typedef struct _hashmap {
	int max_;
	int size_;
	hash_node *data;
} HashMap;

typedef int(*iter_func)(void*, void*);

HashMap* hashmap_new();
int hashmap_iterate(HashMap* m, iter_func f, void* item);
int hashmap_put(HashMap* m, char* key, size_t len, void* value);
int hashmap_get(HashMap* m, char* key, size_t len, void** arg);
int hashmap_remove(HashMap* m, char* key, size_t len);
void hashmap_free(HashMap* m);
