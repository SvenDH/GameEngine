#pragma once
#include "types.h"
#include "memory.h"

//#include <uv.h>
#include <assert.h>

typedef struct {
	allocator_t;
	size_t element_size;
} vector_t;

typedef struct {
	allocator_t;
} hashmap_t;

typedef struct {
	buffer_t;
	size_t element_size;
} queue_t;

typedef struct {
	key_t key;
	int count;
	void* data;
} hash_node;

#define hashmap_put(_m, _k, _v) _hashmap_put((_m), (key_t)(_k), (void*)(_v))
#define hashmap_get(_m, _k) _hashmap_get((_m), (key_t)(_k))
#define hashmap_remove(_m, _k) _hashmap_remove((_m), (key_t)(_k))
#define hashmap_foreach(_m, _key, _value) \
	for (int _i = 0; _i < (_m)->cap / sizeof(hash_node); _i++) \
		if (((hash_node*)(_m)->data)[_i].count > 0, \
			(_key) = ((hash_node*)(_m)->data)[_i].key, \
			(_value) = ((hash_node*)(_m)->data)[_i].data)

void hashmap_init(hashmap_t* map);
int _hashmap_put(hashmap_t* m, key_t key, void* value);
void* _hashmap_get(hashmap_t* m, key_t key);
int _hashmap_remove(hashmap_t* m, key_t key);
void hashmap_free(hashmap_t* m);

void vector_init(vector_t* vec, size_t element_size);
void vector_add(vector_t* vec, void* ptr);
void vector_set(vector_t* vec, size_t index, void* ptr);
void* vector_get(vector_t* vec, size_t index);
void vector_remove(vector_t* vec, void* ptr);
int vector_pop(vector_t* vec, void* ptr);
void vector_free(vector_t* vec);

void queue_init(queue_t* q, size_t element_size);
int queue_put(queue_t* q, void* data);
int queue_get(queue_t* q, void* data);

/*
#define HANDLE(_ui32) (Handle){.value = (_ui32)}
#define NULL_HANDLE HANDLE(0)
#define MAX_HANDLE_ENTRIES 4096

typedef union {
	struct {
		uint32_t index : 12;
		uint32_t counter : 15;
		uint32_t type : 5;
	};
	uint32_t value;
} Handle;

typedef struct {
	uint32_t next : 12;
	uint32_t counter : 15;
	uint32_t active : 1;
	uint32_t end : 1;
	void* data;
} handle_entry;

typedef struct {
	handle_entry entries[MAX_HANDLE_ENTRIES];
	int count;
	uint32_t free_list;
} handle_manager;

handle_manager* handlemanager_init();
Handle handlemanager_add(handle_manager* manager, void* data, uint32_t type);
void handlemanager_update(handle_manager* manager, Handle handle, void* data);
void handlemanager_remove(handle_manager* manager, Handle handle);
int handlemanager_lookup(handle_manager* manager, Handle handle, void** out);
void* handlemanager_get(handle_manager* manager, Handle handle);
*/