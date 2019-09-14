#include "memory.h"

void objectallocator_init(ObjectAllocator* ob, const char* name, size_t element_size, size_t block_size, size_t allign) {
	memoryuser_init(ob, name);
	ALLOCATOR_INIT(ob, memoryuser_alloc(ob, block_size * element_size, allign), block_size * element_size);
	ob->element_size = element_size;
	ob->block_size = block_size * element_size;
	ob->allign = allign;
	objectallocator_clear(ob);
	//TODO: make element_size and block_size powers of 2
}

void* objectallocator_alloc(ObjectAllocator* ob, size_t size) {
	assert(size == ob->element_size);
	if (ob->free_list) {
		void* recycle = ob->free_list;
		ob->free_list = *(void**)ob->free_list;
		return recycle;
	}

	char** blocks = &ob->data;
	int nr_blocks = ob->cap / ob->block_size;
	if (nr_blocks > 1)
		blocks = *blocks;
	//If there are more then one block we store a list of blocks at ob->data else we store the block at ob->data
	if (ob->used % ob->block_size == 0) { //TODO: change modulo to >>
		if (++ob->block_index == nr_blocks) {
			char** old_blocks = blocks;
			blocks = calloc(nr_blocks * 2, sizeof(char*));
			ob->data = memcpy(blocks, old_blocks, nr_blocks * sizeof(char*));
			ob->cap *= 2;
			if (nr_blocks > 1) free(old_blocks);
		}
		if (blocks[ob->block_index] == NULL)
			blocks[ob->block_index] = memoryuser_alloc(ob, ob->block_size, ob->allign);
	}

	void* ptr = blocks[ob->block_index] + (ob->used % ob->block_size);
	ob->used += ob->element_size;
	return ptr;
}

void objectallocator_free(ObjectAllocator* ob, void* ptr) {
	*(void**)ptr = ob->free_list;
	ob->free_list = ptr;
}

void objectallocator_clear(ObjectAllocator* ob) {
	ob->used = 0;
	ob->block_index = 0;
	ob->free_list = NULL;
}