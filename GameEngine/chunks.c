#include "ecs.h"

Chunk* chunk_new(World* wld, type_t type, entity_t parent) {
	Archetype* archetype = archetype_get(wld, type, 1);
	Chunk* new_chunk = objectallocator_alloc(&wld->chunks, CHUNK_SIZE);
	assert(((intptr_t)new_chunk & CHUNK_SIZE - 1) == 0);
	new_chunk->id = wld->chunk_index++;
	new_chunk->type = type;
	new_chunk->parent = parent;
	new_chunk->ent_count = 0;
	new_chunk->max_ents = archetype->max_ents;
	new_chunk->prev_children = 0;
	new_chunk->next_children = 0;
	memcpy(new_chunk->sizes, archetype->sizes, MAX_COMPS); //TODO: get sizes from world
	archetype->chunk_count++;

	hashmap_put(&wld->chunk_map, new_chunk->id, new_chunk);
	hashmap_put(&archetype->chunks, new_chunk->parent, new_chunk);

	if (parent) {
		ent_container* container_ptr = entity_getcomponent(wld, parent, comp_container);
		if (!container_ptr) { //if parent does not have container component add it
			if (!entity_changetype(wld, parent, entity_gettype(wld, parent) | container))
				container_ptr = entity_getcomponent(wld, parent, comp_container);
		}

		if (container_ptr->children) {
			Chunk* next_chunk = hashmap_get(&wld->chunk_map, container_ptr->children);
			next_chunk->prev_children = new_chunk->id;
			new_chunk->next_children = next_chunk->id;
		}
		container_ptr->children = new_chunk->id;
	}

	return new_chunk;
}

Chunk* chunk_get(World* wld, type_t type, entity_t parent, int create) {
	Archetype* archetype = archetype_get(wld, type, create);
	Chunk* chunk = hashmap_get(&archetype->chunks, parent); //TODO: link chunks, make new chunks if last one is full
	if (create && !chunk)
		chunk = chunk_new(wld, type, parent);
	
	return chunk;
}

void chunk_delete(World* wld, Chunk* chunk) {
	// Delete all entities
	entity_t* ent_ids = chunk_get_componentarray(chunk, 0);
	for (int i = chunk->ent_count - 1; i >= 0; i--)
		entity_delete(wld, ent_ids[i]);

	if (chunk->parent) {
		entity_t parent = chunk->parent;
		ent_container* container_ptr = entity_getcomponent(wld, parent, comp_container);
		if (container_ptr->children == chunk->id) container_ptr->children = chunk->next_children;
	}

	if (chunk->next_children) {
		Chunk* next_chunk = hashmap_get(&wld->chunk_map, chunk->next_children);
		next_chunk->prev_children = chunk->prev_children;
	}

	if (chunk->prev_children) {
		Chunk* prev_chunk = hashmap_get(&wld->chunk_map, chunk->prev_children);
		prev_chunk->next_children = chunk->next_children;
	}

	Archetype* archetype = archetype_get(wld, chunk->type, 0);
	hashmap_remove(&archetype->chunks, chunk->parent);
	archetype->chunk_count--;

	hashmap_remove(&wld->chunk_map, chunk->id);
	objectallocator_free(&wld->chunks, chunk);

	if (!archetype->chunk_count)
		archetype_delete(wld, archetype);
}

void* chunk_get_componentarray(Chunk* chunk, int component) {
	if (!(chunk->type & 1 << component)) return NULL;
	intptr_t loc = 0;
	for (int i = 0; i < component; i++)
		loc += chunk->max_ents * chunk->sizes[i];

	return (void*)&chunk->data[loc];
}

void* chunk_get_component(Chunk* chunk, int component, int index) {
	intptr_t loc = chunk_get_componentarray(chunk, component);
	if (loc) loc += index * chunk->sizes[component];

	return (void*)loc;
}
