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

chunk_data* chunk_get(world_t* wld, chunk_t id) {
	//TODO: load from disk if needed
	return hashmap_get(&wld->chunk_map, id);
}

chunk_data* chunk_find(world_t* wld, type_t type, entity_t parent, int create) {
	chunk_data* chunk = NULL;
	if (type) {
		chunk_t searchchunk;
		//Search for chunk with the correct parent, TODO: sort or make finding chunk easier
		type_data* archetype = archetype_get(wld, type, create);
		archetype_foreach(wld, archetype, searchchunk) {
			chunk = chunk_get(wld, searchchunk);
			if (chunk->parent == parent && chunk->ent_count < chunk->max_ents)
				break;
			chunk = NULL;
		}
		if (create && !chunk)
			chunk = chunk_new(wld, type, parent);
	}	
	return chunk;
}

void chunk_link(world_t* wld, chunk_data* chunk) {
	//Link archetype chunks
	type_data* archetype = archetype_get(wld, chunk->type, 1); //TODO: group by parent
	if (archetype->chunk_list) {
		chunk_data* next_chunk = chunk_get(wld, archetype->chunk_list);
		next_chunk->prev_chunk = chunk->id;
		chunk->next_chunk = next_chunk->id;
	}
	archetype->chunk_list = chunk->id;
	archetype->chunk_count++;
	
	//Link child to parents
	entity_t parent = chunk->parent;
	if (parent) { //TODO: group by archetype
		Container* container_ptr = entity_getcomponent(wld, parent, COMP_TYPE(Container));
		if (!container_ptr &&
			!entity_changetype(wld, parent, entity_gettype(wld, parent) | COMP_FLAG(Container))) //if parent does not have container component add it
			container_ptr = entity_getcomponent(wld, parent, COMP_TYPE(Container));

		if (container_ptr->children) {
			chunk_data* next_chunk = chunk_get(wld, container_ptr->children);
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
		chunk_data* next_chunk = chunk_get(wld, chunk->next_chunk);
		next_chunk->prev_chunk = chunk->prev_chunk;
	}
	if (chunk->prev_chunk) {
		chunk_data* prev_chunk = chunk_get(wld, chunk->prev_chunk);
		prev_chunk->next_chunk = chunk->next_chunk;
	}

	//Relink children of parent
	if (chunk->parent) {
		entity_t parent = chunk->parent;
		Container* container_ptr = entity_getcomponent(wld, parent, COMP_TYPE(Container));
		if (container_ptr->children == chunk->id) 
			container_ptr->children = chunk->next_children;
	}
	if (chunk->next_children) {
		chunk_data* next_chunk = chunk_get(wld, chunk->next_children);
		next_chunk->prev_children = chunk->prev_children;
	}
	if (chunk->prev_children) {
		chunk_data* prev_chunk = chunk_get(wld, chunk->prev_children);
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
	entity_t* ent_ids = chunk_getcomponentarray(wld, chunk, 0);
	for (int i = chunk->ent_count - 1; i >= 0; i--)
		entity_delete(wld, ent_ids[i]);

	chunk_relink(wld, chunk);

	hashmap_remove(&wld->chunk_map, chunk->id);
	object_free(&wld->chunks, chunk);
	//TODO: save to disk
}

int chunk_componentcopy(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src) {
	assert((dest_chunk && dest < dest_chunk->max_ents && src >= 0) || !dest_chunk);
	assert((src_chunk && src < src_chunk->max_ents && src >= 0) || !src_chunk);
	// Copy all components to new location
	type_t dest_type = dest_chunk ? dest_chunk->type : 0;
	type_t src_type = src_chunk ? src_chunk->type : 0;
	for (int i = 0; i < wld->component_counter; i++) {
		void* comp = chunk_getcomponent(wld, dest_chunk, i, dest);
		if ((1 << i) & src_type & dest_type)
			memcpy(comp, chunk_getcomponent(wld, src_chunk, i, src), wld->component_info[i].size);
		else if ((1 << i) & ~src_type & dest_type)
			memset(comp, 0, wld->component_info[i].size);
	}
	return 0;
}

int chunk_componentmove(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src) {
	chunk_componentcopy(wld, dest_chunk, src_chunk, dest, src);
	if (dest_chunk)
		dest_chunk->ent_count++;
	if (src_chunk && --src_chunk->ent_count == 0)
		chunk_delete(wld, src_chunk);

	return 0;
}

void* chunk_getcomponentarray(world_t* wld, chunk_data* chunk, int component) {
	if (!chunk || !(chunk->type & 1<<component)) 
		return NULL;
	int loc = 0;
	for (int i = 0; i < component; i++)
		loc += (((1<<i) & chunk->type) > 0) * chunk->max_ents * wld->component_info[i].size;

	return (void*)&chunk->data[loc];
}

void* chunk_getcomponent(world_t* wld, chunk_data* chunk, int component, int index) {
	char* loc = (char*)chunk_getcomponentarray(wld, chunk, component);
	if (loc) 
		loc += index * wld->component_info[component].size;

	return (void*)loc;
}

chunk_data* component_to_chunk(world_t* wld, void* comp_ptr, int comp_type, int* index) {
	intptr_t loc = (intptr_t)comp_ptr; //Find chunk using allignment of chunk
	chunk_data* chk = (chunk_data*)(loc - (loc % CHUNK_SIZE));
	if (index) {
		intptr_t arr = (intptr_t)chunk_getcomponentarray(wld, chk, comp_type);
		*index = (loc - arr) / wld->component_info[comp_type].size;
	}
	return chk;
}
