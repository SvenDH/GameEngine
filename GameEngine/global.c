#include "data.h"
#include "utils.h"

MemoryManager* memorymanager_instance() {
	static MemoryManager* manager;
	if (!manager) {
		manager = (MemoryManager*)malloc(sizeof(MemoryManager));
		ALLOCATOR_INIT(manager, aligned_alloc(PAGE_SIZE, GLOBAL_BUFFER_SIZE), GLOBAL_BUFFER_SIZE);
		assert(manager->data);
	}
	return manager;
}

void* global_alloc(MemoryManager* manager, size_t size, size_t allign, const char* name) {
	assert(size > 0 && "allocate called with size = 0.");
	//TODO: register name
	intptr_t ptr = &manager->data[manager->used];
	int adjustment = allign_ptr(ptr, allign, sizeof(int));
	if (manager->used + size + adjustment > manager->cap) return NULL;

	*(int*)ptr = adjustment;
	ptr += adjustment;
	manager->used += size + adjustment;

	return (void*)ptr;
}

void global_free(MemoryManager* manager, void* mem) {
	intptr_t ptr = (intptr_t)mem;
	ptr -= sizeof(int);
	manager->used -= ((intptr_t)&manager->data[manager->used]) - ((intptr_t)mem + *(int*)ptr);
}

void global_clear(MemoryManager* manager) {
	manager->used = 0;
}
