#include "ecs.h"

entity_t entity_new(world_t* wld, entity_t ent, type_t type, entity_t parent) {
	entity_t new_entity;
	if (ent) new_entity = ent;
	else random_uid(&new_entity);
	//TODO: what to do if ent already exists? now it creates new entity
	while (hashmap_get(&wld->entity_map, new_entity))
		random_uid(&new_entity);
	//Make sure we also include id
	type = type|id;

	entity_change(wld, new_entity, parent, type);

	*(entity_t*)entity_getcomponent(wld, new_entity, comp_id) = new_entity;

	return new_entity;
}

entity_t* entity_get(world_t* wld, entity_t id) {
	//TODO: caching previously searched entity
	return hashmap_get(&wld->entity_map, id);
}

void* entity_getcomponent(world_t* wld, entity_t ent, int component) {
	int index;
	chunk_data* chk = entity_to_chunk(wld, ent, &index);
	return chunk_get_component(chk, component, index);
}

chunk_data* entity_to_chunk(world_t* wld, entity_t ent, int* index) {
	entity_t* loc = entity_get(wld, ent);
	if (loc) return component_to_chunk(loc, comp_id, index);
	else if (index) *index = 0;
	return NULL;
}

int entity_change(world_t* wld, entity_t ent, entity_t new_parent, type_t new_type) {
	int index;
	chunk_data* new_chk = chunk_get(wld, new_type, new_parent, 1);
	chunk_data* chk = entity_to_chunk(wld, ent, &index);
	if (new_chk != chk && !entity_move(wld, ent, new_chk, new_chk->ent_count)) {
		if (chk && index != chk->ent_count) //TODO: figure out a deferred way to prevent moving
			entity_move(wld, *(entity_t*)chunk_get_component(chk, comp_id, chk->ent_count), chk, index);
		//Swap last entity to original position
		return 0;
	} //No space 
	return -1;
}

int entity_move(world_t* wld, entity_t ent, chunk_data* dest_chunk, int dest) {
	//Copy entity to new location
	int src;
	chunk_data* src_chunk = entity_to_chunk(wld, ent, &src);
	type_t dest_type = dest_chunk ? dest_chunk->type : 0;
	type_t src_type = src_chunk ? src_chunk->type : 0;

	ecs_delcomponents_cb(wld, ent, src_type & ~dest_type);

	chunk_componentmove(wld, dest_chunk, src_chunk, dest, src);

	// Change entity location
	entity_t* swap_id = chunk_get_component(dest_chunk, comp_id, dest);
	if (swap_id) hashmap_put(&wld->entity_map, ent, swap_id);

	ecs_addcomponents_cb(wld, ent, ~src_type & dest_type);

	return 0;
}

void entity_delete(world_t* wld, entity_t ent) {
	entity_change(wld, ent, 0, 0);
	hashmap_remove(&wld->entity_map, ent);
}