#include "data.h"

#define INITIAL_ELEMENTS 16

void vector_init(vector_t* vec, size_t element_size) {
	ALLOCATOR_INIT(vec, malloc(element_size * INITIAL_ELEMENTS), element_size * INITIAL_ELEMENTS);
	vec->element_size = element_size;
}

void vector_add(vector_t* vec, void* ptr) {
	if (vec->used == vec->cap) {
		vec->cap *= 2;
		vec->data = realloc(vec->data, vec->cap);
	}
	memcpy(&vec->data[vec->used], ptr, vec->element_size);
	vec->used += vec->element_size;
}

void vector_set(vector_t* vec, size_t index, void* ptr) {
	assert(index * vec->element_size < vec->used);
	memcpy(&vec->data[index * vec->element_size], ptr, vec->element_size);
}

void* vector_get(vector_t* vec, size_t index) {
	return &vec->data[index * vec->element_size];
}

void vector_remove(vector_t* vec, void* ptr) {
	size_t index = ((char*)ptr - vec->data) / vec->element_size;
	assert(index * vec->element_size < vec->used);
	//Swap last element
	memcpy(ptr, &vec->data[vec->used - vec->element_size], vec->element_size);
	vec->used -= vec->element_size;
}

bool vector_pop(vector_t* vec, void* ptr) {
	if (!vec->used) return 0;
	if (ptr) 
		memcpy(ptr, &vec->data[vec->used - vec->element_size], vec->element_size);
	vec->used -= vec->element_size;
	return true;
}

void vector_free(vector_t* vec) {
	free(vec->data);
}