#pragma once
//#include "types.h"
#include <stdint.h>
#include <assert.h>

#define GLOBAL_BUFFER_SIZE 134217728 //128MB

#define ALLOCATOR_INIT(_buf, _data, _size)	do {(_buf)->data = _data; (_buf)->cap = _size; (_buf)->used = 0;} while (0)

#define allocator_isempty(_buf) ((_buf)->used == 0)
#define allocator_isfull(_buf)	((_buf)->cap == (_buf)->used)
#define allocator_unused(_buf)  ((_buf)->cap - (_buf)->used)

typedef struct {
	char* data;
	size_t cap;
	size_t used;
} allocator_t;

typedef struct {
	allocator_t;
	size_t element_size;
	void** free_list;
} pool_t;

typedef struct {
	allocator_t;
	size_t read, write, end;
} buffer_t;

typedef struct {
	allocator_t;
} MemoryManager;

typedef struct {
	allocator_t;
	MemoryManager* _global_mm;
	char* name;
} memory_user_t;

typedef struct {
	memory_user_t;
	size_t element_size;
	size_t block_size;
	size_t allign;
	int block_index;
	void** free_list;
} ObjectAllocator;

void bip_init(buffer_t* b, void* data, size_t size);
void* bip_alloc(buffer_t* b, size_t size);
void bip_free(buffer_t* b, void* ptr);
void bip_clear(buffer_t* b);

int bip_write(buffer_t* b, size_t size);
void* bip_peek(buffer_t* b, size_t size);
int bip_read(buffer_t* b, size_t size);

MemoryManager* memorymanager_instance();
void* global_alloc(MemoryManager* manager, size_t size, size_t allign, const char* name);
void global_free(MemoryManager* manager, void* mem);
void global_clear(MemoryManager* manager);

inline void memoryuser_init(memory_user_t* mu, const char* name) {
	mu->_global_mm = memorymanager_instance();
	mu->name = name;
}

inline void* memoryuser_alloc(memory_user_t* mu, size_t size, size_t allign) {
	return global_alloc(mu->_global_mm, size, allign, mu->name);
}

inline void memoryuser_free(memory_user_t* mu, void* mem) {
	return global_free(mu->_global_mm, mem);
}

void objectallocator_init(ObjectAllocator* ob, const char* name, size_t element_size, size_t block_size, size_t allign);
void* objectallocator_alloc(ObjectAllocator* ob, size_t size);
void objectallocator_free(ObjectAllocator* ob, void* ptr);
void objectallocator_clear(ObjectAllocator* ob);