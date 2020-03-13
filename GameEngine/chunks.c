#include "ecs.h"

chunk_data* chunk_new(world_t* wld, type_t type, entity_t parent) {
	type_data* archetype = archetype_get(wld, type, 1);
	chunk_data* new_chunk = object_alloc(&wld->chunks, CHUNK_SIZE);
	assert(((intptr_t)new_chunk & CHUNK_SIZE - 1) == 0);
	new_chunk->id = wld->chunk_index++;
	new_chunk->type = type;
	new_chunk->parent = parent;
	new_chunk->ent_count = 0;
	new_chunk->max_ents = archetype->max_ents;
	new_chunk->prev_children = 0;
	new_chunk->next_children = 0;
	new_chunk->prev_chunk = 0;
	new_chunk->next_chunk = 0;
	hashmap_put(&wld->chunk_map, new_chunk->id, new_chunk);
	chunk_link(wld, new_chunk);

	return new_chunk;
}

chunk_data* chunk_find(world_t* wld, chunk_t id) {
	//TODO: load from disk if needed
	return hashmap_get(&wld->chunk_map, id);
}

chunk_data* chunk_get(world_t* wld, type_t type, entity_t parent, int create) {
	type_data* archetype = archetype_get(wld, type, create);
	chunk_data* chunk = NULL;
	chunk_data* searchchunk;
	//Search for chunk with the correct parent, TODO: sort or make finding chunk easier
	archetype_foreach(wld, archetype, searchchunk) {
		if (searchchunk->parent == parent &&
			searchchunk->ent_count < searchchunk->max_ents) {
			chunk = searchchunk;
			break;
		}
	}
	if (create && !chunk)
		chunk = chunk_new(wld, type, parent);
	
	return chunk;
}

void chunk_link(world_t* wld, chunk_data* chunk) {
	//Link archetype chunks
	type_data* archetype = archetype_get(wld, chunk->type, 1); //TODO: group by parent
	if (archetype->chunk_list) {
		chunk_data* next_chunk = chunk_find(wld, archetype->chunk_list);
		next_chunk->prev_chunk = chunk->id;
		chunk->next_chunk = next_chunk->id;
	}
	archetype->chunk_list = chunk->id;
	archetype->chunk_count++;
	
	//Link child to parents
	entity_t parent = chunk->parent;
	if (parent) { //TODO: group by archetype
		ent_container* container_ptr = entity_getcomponent(wld, parent, comp_container);
		if (!container_ptr &&
			!entity_changetype(wld, parent, entity_gettype(wld, parent) | container)) //if parent does not have container component add it
			container_ptr = entity_getcomponent(wld, parent, comp_container);

		if (container_ptr->children) {
			chunk_data* next_chunk = chunk_find(wld, container_ptr->children);
			next_chunk->prev_children = chunk->id;
			chunk->next_children = next_chunk->id;
		}
		container_ptr->children = chunk->id;
	}
}

void chunk_relink(world_t* wld, chunk_data* chunk) {
	//Relink archetype list
	type_data* archetype = archetype_get(wld, chunk->type, 0);
	if (archetype->chunk_list == chunk->id)
		archetype->chunk_list = chunk->next_chunk;
	
	if (--archetype->chunk_count == 0)
		archetype_delete(wld, archetype);

	if (chunk->next_chunk) {
		chunk_data* next_chunk = chunk_find(wld, chunk->next_chunk);
		next_chunk->prev_chunk = chunk->prev_chunk;
	}
	if (chunk->prev_chunk) {
		chunk_data* prev_chunk = chunk_find(wld, chunk->prev_chunk);
		prev_chunk->next_chunk = chunk->next_chunk;
	}

	//Relink children of parent
	if (chunk->parent) {
		entity_t parent = chunk->parent;
		ent_container* container_ptr = entity_getcomponent(wld, parent, comp_container);
		if (container_ptr->children == chunk->id) 
			container_ptr->children = chunk->next_children;
	}
	if (chunk->next_children) {
		chunk_data* next_chunk = chunk_find(wld, chunk->next_children);
		next_chunk->prev_children = chunk->prev_children;
	}
	if (chunk->prev_children) {
		chunk_data* prev_chunk = chunk_find(wld, chunk->prev_children);
		prev_chunk->next_children = chunk->next_children;
	}
}

int chunk_change(world_t* wld, chunk_data* chunk, type_t new_type, entity_t new_parent) {
	if (chunk->parent != new_parent || chunk->type != new_type) {
		chunk_relink(wld, chunk);
		chunk->parent = new_parent;
		chunk->type = new_type;
		chunk_link(wld, chunk);
		return 0;
	}
	return -1;
}

void chunk_delete(world_t* wld, chunk_data* chunk) {
	// Delete all entities
	entity_t* ent_ids = chunk_get_componentarray(chunk, 0);
	for (int i = chunk->ent_count - 1; i >= 0; i--)
		entity_delete(wld, ent_ids[i]);

	chunk_relink(wld, chunk);

	hashmap_remove(&wld->chunk_map, chunk->id);
	object_free(&wld->chunks, chunk);
	//TODO: save to disk
}

int chunk_componentcopy(chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src) {
	assert((dest_chunk && dest < dest_chunk->max_ents && src >= 0) || !dest_chunk);
	assert((src_chunk && src < src_chunk->max_ents && src >= 0) || !src_chunk);
	// Copy all components to new location
	type_t dest_type = dest_chunk ? dest_chunk->type : 0;
	type_t src_type = src_chunk ? src_chunk->type : 0;
	for (int i = 0; i < NR_COMPS; i++) {
		if ((1 << i) & src_type & dest_type) {
			memcpy(
				chunk_get_component(dest_chunk, i, dest),
				chunk_get_component(src_chunk, i, src),
				component_sizes[i]);
		}
		else if ((1 << i) & ~src_type & dest_type) {
			memcpy(
				chunk_get_component(dest_chunk, i, dest),
				component_default[i],
				component_sizes[i]);
		}
	}
	return 0;
}

int chunk_componentmove(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src) {
	chunk_componentcopy(dest_chunk, src_chunk, dest, src);
	dest_chunk->ent_count++;

	if (src_chunk && --src_chunk->ent_count == 0)
		chunk_delete(wld, src_chunk);

	return 0;
}

void* chunk_get_componentarray(chunk_data* chunk, int component) {
	if (!(chunk->type & 1 << component)) return NULL;
	int loc = 0;
	for (int i = 0; i < component; i++)
		loc += (((1<<i) & chunk->type) > 0) * chunk->max_ents * component_sizes[i];

	return (void*)&chunk->data[loc];
}

void* chunk_get_component(chunk_data* chunk, int component, int index) {
	char* loc = (char*)chunk_get_componentarray(chunk, component);
	if (loc) loc += index * component_sizes[component];

	return (void*)loc;
}
