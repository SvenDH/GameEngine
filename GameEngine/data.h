#pragma once
#include "types.h"
#include "memory.h"
#include <stdint.h>
#include <assert.h>


#define GLOBAL_BUFFER_SIZE 134217728 //128MB

#define ALLOCATOR_INIT(_buf, _data, _size)	do {(_buf)->data = _data; (_buf)->cap = _size; (_buf)->used = 0;} while (0)

#define allocator_isempty(_buf) ((_buf)->used == 0)
#define allocator_isfull(_buf)	((_buf)->cap == (_buf)->used)
#define allocator_unused(_buf)  ((_buf)->cap - (_buf)->used)

#define list_foreach(_list, _node) \
	for ((_node) = (_list)->next; (_list) != (_node); (_node) = (_node)->next )

#define hashmap_put(_m, _k, _v) _hashmap_put((_m), (key_t)(_k), (void*)(_v))
#define hashmap_get(_m, _k) _hashmap_get((_m), (key_t)(_k))
#define hashmap_isempty(_m) allocator_isempty(_m)
#define hashmap_remove(_m, _k) _hashmap_remove((_m), (key_t)(_k))
#define hashmap_foreach(_m, _key, _value) \
	if (!allocator_isempty(_m)) \
		for (uint32_t _im = 0; _im < (_m)->cap / sizeof(hash_node); _im++) \
			if (((hash_node*)(_m)->data)[_im].count > 0, \
				(_key) = ((hash_node*)(_m)->data)[_im].key, \
				(_value) = ((hash_node*)(_m)->data)[_im].data)

#define vector_foreach(_v, _type, _value) \
	if (!allocator_isempty(_v)) \
		for (uint32_t _iv = 0, _value = *((_type*)(_v)->data); \
			_iv < (_v)->used / sizeof(_type); \
			_iv++, (_value) = ((_type*)(_v)->data)[_iv])

#define circbuffer_full(_b, _size) (((_b)->head + 1) % (_size) == (_b)->tail)
#define circbuffer_empty(_b, _size) ((_b)->head == (_b)->tail)
#define circbuffer_push(_b, _val, _size) do {\
	if circbuffer_full(_b, _size) break; \
	int _next = (_b)->head + 1; \
	if (_next >= (_size)) _next = 0; \
	(_b)->buffer[(_b)->head] = _val; \
	(_b)->head = _next; } while(0)
#define circbuffer_pop(_b, _val, _size) do {\
	if circbuffer_empty(_b, _size) break; \
	int _next = (_b)->tail + 1; \
	if (_next >= _size) _next = 0; \
	_val = (_b)->buffer[(_b)->tail]; \
	(_b)->tail = _next; } while(0)

typedef struct listnode_t {
	struct listnode_t* prev;
	struct listnode_t* next;
} listnode_t;

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
} memorymanager_t;

typedef struct {
	allocator_t;
	memorymanager_t* _global_mm;
	const char* name;
} memory_user_t;

typedef struct {
	memory_user_t;
	size_t element_size;
	size_t block_size;
	size_t allign;
	int block_index;
	void** free_list;
	bool initialized;
} object_allocator_t;

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

inline void list_init(listnode_t* list) {
	list->prev = list; 
	list->next = list;
}

inline void list_add(struct listnode_t* list, struct listnode_t* node) {
	node->prev = list->prev;
	node->next = list;
	list->prev->next = node;
	list->prev = node;
}

inline void list_remove(listnode_t* list, listnode_t* node) {
	node->next->prev = node->prev;
	node->prev->next = node->next; 
}

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
bool vector_pop(vector_t* vec, void* ptr);
void vector_free(vector_t* vec);

void queue_init(queue_t* q, size_t element_size);
int queue_put(queue_t* q, void* data);
int queue_get(queue_t* q, void* data);

void bip_init(buffer_t* b, void* data, size_t size);
void* bip_alloc(buffer_t* b, size_t size);
void bip_free(buffer_t* b, void* ptr);
void bip_clear(buffer_t* b);

int bip_write(buffer_t* b, size_t size);
void* bip_peek(buffer_t* b, size_t size);
int bip_read(buffer_t* b, size_t size);

memorymanager_t* memorymanager_instance();
void* global_alloc(memorymanager_t* manager, size_t size, size_t allign, const char* name);
void global_free(memorymanager_t* manager, void* mem);
void global_clear(memorymanager_t* manager);

inline void memoryuser_init(memory_user_t* mu, const char* name) {
	mu->_global_mm = memorymanager_instance();
	mu->name = name;
}

inline void* memoryuser_alloc(memory_user_t* mu, size_t size, size_t allign) {
	return global_alloc(mu->_global_mm, size, allign, mu->name);
}

inline void memoryuser_free(memory_user_t* mu, void* mem) {
	global_free(mu->_global_mm, mem);
}

void object_init(object_allocator_t* ob, const char* name, size_t element_size, size_t block_size, size_t allign);
void* object_alloc(object_allocator_t* ob, size_t size);
void object_free(object_allocator_t* ob, void* ptr);
void object_clear(object_allocator_t* ob);

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

typedef void* (*pair_cb)(UID, UID, void*);
typedef void (*unpair_cb)(void*, void*);
typedef struct {
	object_allocator_t nodes;
	object_allocator_t objects;
	object_allocator_t pairs;
	hashmap_t objectmap;
	hashmap_t bodygrid;
	int tile_size;
	int pass;
	void* ud;
	pair_cb pair;
	unpair_cb unpair;
} hashgrid_t;

void hashgrid_new(hashgrid_t* spc, uint16_t tile_size, pair_cb pair, unpair_cb unpair, void* ud);
void hashgrid_delete(hashgrid_t* spc);
void hashgrid_create(hashgrid_t* spc, UID id);
void hashgrid_remove(hashgrid_t* spc, UID id);
void hashgrid_move(hashgrid_t* spc, UID id, rect2 aabb, bool collide);
int hashgrid_cullray(hashgrid_t* spc, vec2 begin, vec2 end, UID* results, int max_results);
int hashgrid_cullaabb(hashgrid_t* spc, rect2 aabb, UID* results, int max_results);
