#include "ecs.h"

entity_t entity_new(World* wld, entity_t ent, type_t type, entity_t parent) {

	entity_t new_entity;
	if (ent) new_entity = ent;
	else random_uid(&new_entity);
	//TODO: what to do if ent already exists? now it creates new entity
	while (hashmap_get(&wld->entity_map, new_entity))
		random_uid(&new_entity);

	Chunk* chunk = chunk_get(wld, type | id, parent, 1);
	if (chunk->ent_count != chunk->max_ents) {
		entity_t* loc = chunk_get_component(chunk, 0, chunk->ent_count++);
		*loc = new_entity;
		hashmap_put(&wld->entity_map, *loc, loc);
		event_post(eventhandler_instance(), (Event) { .type = EVT_NEWENTITY, .data = new_entity });
		return new_entity;
	}
	return 0;
}

entity_t* entity_get(World* wld, entity_t id) {
	//TODO: caching previously searched entity
	return hashmap_get(&wld->entity_map, id);
}

void* entity_getcomponent(World* wld, entity_t ent, int component) {
	int index;
	Chunk* chk = entity_to_chunk(wld, ent, &index);
	return chunk_get_component(chk, component, index);
}

Chunk* entity_to_chunk(World* wld, entity_t ent, int* index) {
	intptr_t loc = entity_get(wld, ent);
	if (loc) { //Find chunk using allignment of chunk
		Chunk* chunk = (Chunk*)(loc - (loc % CHUNK_SIZE));
		if (index) *index = (loc - (intptr_t)&chunk->data) / sizeof(entity_t);
		return chunk;
	}
	return NULL;
}

int entity_move(World* wld, Chunk* dest_chunk, Chunk* src_chunk, int dest, int src) {
	//Copy entity to new location
	if (entity_copy(dest_chunk, src_chunk, dest, src)) return -1;
	// Change entity location
	entity_t* swap_id = chunk_get_component(dest_chunk, comp_id, dest);
	hashmap_put(&wld->entity_map, *swap_id, swap_id);
	if (src_chunk != dest_chunk) {
		src_chunk->ent_count--;
		dest_chunk->ent_count++;
		if (src_chunk->parent != dest_chunk->parent) {
			if (src_chunk->parent) ((ent_container*)entity_getcomponent(wld, src_chunk->parent, comp_container))->nr_children--;
			if (dest_chunk->parent) ((ent_container*)entity_getcomponent(wld, dest_chunk->parent, comp_container))->nr_children++;
		}
	}
	if (!src_chunk->ent_count)
		chunk_delete(wld, src_chunk);
	return 0;
}

int entity_change(World* wld, entity_t ent, entity_t new_parent, type_t new_type) {
	int index;
	Chunk* new_chk = chunk_get(wld, new_type, new_parent, 1);
	Chunk* chk = entity_to_chunk(wld, ent, &index);
	if (new_chk != chk && !entity_move(wld, new_chk, chk, new_chk->ent_count, index)) {
		if (index != chk->ent_count) //TODO: figure out a deferred way to prevent moving
			entity_move(wld, chk, chk, index, chk->ent_count);
		//Swap last entity to original position
		return 0;
	} //No space 
	return -1;
}

void entity_delete(World* wld, entity_t ent) {
	int index;
	Chunk* chunk = entity_to_chunk(wld, ent, &index);
	if (index != --chunk->ent_count)
		entity_move(wld, chunk, chunk, index, chunk->ent_count);

	event_post(eventhandler_instance(), (Event) { .type = EVT_DELENTITY, .data = ent });
	hashmap_remove(&wld->entity_map, ent);

	if (!chunk->ent_count)
		chunk_delete(wld, chunk->type, chunk);
}

int entity_copy(Chunk* dest_chunk, Chunk* src_chunk, int dest, int src) {
	if (dest >= dest_chunk->max_ents || src >= src_chunk->max_ents) return -1;
	// Copy all components to new location
	for (int i = 0; i < NR_COMPS; i++) {
		if ((1 << i) & src_chunk->type & dest_chunk->type) {
			memcpy(
				chunk_get_component(dest_chunk, i, dest),
				chunk_get_component(src_chunk, i, src),
				src_chunk->sizes[i]);
		}
		else if ((1 << i) & ~src_chunk->type & dest_chunk->type) {
			memset(
				chunk_get_component(dest_chunk, i, dest), 
				0, 
				dest_chunk->sizes[i]); //TODO: trigger component create event
		}
	}
	return 0;
}