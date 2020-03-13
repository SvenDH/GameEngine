#include "ecs.h"


type_data* archetype_new(world_t* wld, type_t components) {
	type_data* new_archetype = object_alloc(&wld->archetypes, sizeof(type_data));
	new_archetype->type = components | id;
	new_archetype->chunk_count = 0;

	int total = 0;
#define X(_name, _size, _attr) \
	if (new_archetype->type & _name) { \
		total += _size * sizeof(ent_##_name); \
		new_archetype->sizes[comp_##_name] = _size; \
	} else new_archetype->sizes[comp_##_name] = 0;
	ENTITY_DATA
#undef X
	new_archetype->max_ents = (CHUNK_SIZE - sizeof(chunk_data) + 4) / total;
	new_archetype->chunk_list = 0;

	log_info("New archetype: %i  Max entities: %i", new_archetype->type, new_archetype->max_ents);

	hashmap_put(&wld->archetype_map, new_archetype->type, new_archetype);
	event_post(eventhandler_instance(), (event_t) { 
		.type = on_addtype, 
		.p0.data = new_archetype->type });
	return new_archetype;
}

type_data* archetype_get(world_t* wld, type_t type, int create) {
	//TODO: cache last used archetype
	type_data* archetype = hashmap_get(&wld->archetype_map, type);
	if (!archetype && create) archetype = archetype_new(wld, type);
	return archetype;
}

void archetype_delete(world_t* wld, type_data* archetype) {
	chunk_data* chunk = NULL;
	archetype_foreach(wld, archetype, chunk) {
		chunk_delete(wld, chunk);
	}

	hashmap_remove(&wld->archetype_map, archetype->type);
	event_post(eventhandler_instance(), (event_t) { 
		.type = on_deltype, 
		.p0.data = archetype->type });
	object_free(&wld->archetypes, archetype);
}