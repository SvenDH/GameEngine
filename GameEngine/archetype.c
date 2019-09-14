#include "ecs.h"
#include "event.h"

Archetype* archetype_new(World* wld, type_t components) {
	Archetype* new_archetype = objectallocator_alloc(&wld->archetypes, sizeof(Archetype));
	new_archetype->type = components | id;
	new_archetype->chunk_count = 0;

	int total = 0;
#define X(name, t, _size) \
	if (new_archetype->type & name) { \
		total += _size; \
		new_archetype->sizes[comp_##name] = _size; \
	} else new_archetype->sizes[comp_##name] = 0;
	ENTITY_DATA
#undef X
	new_archetype->max_ents = (CHUNK_SIZE - sizeof(Chunk) + 4) / total;
	hashmap_init(&new_archetype->chunks);

	log_info("New archetype: %i  Max entities: %i", new_archetype->type, new_archetype->max_ents);

	hashmap_put(&wld->archetype_map, new_archetype->type, new_archetype);
	event_post(eventhandler_instance(), (Event) { .type = EVT_NEWTYPE, .data = new_archetype->type });
	return new_archetype;
}

Archetype* archetype_get(World* wld, type_t type, int create) {
	//TODO: cache last used archetype
	Archetype* archetype = hashmap_get(&wld->archetype_map, type);
	if (!archetype && create) archetype = archetype_new(wld, type);
	return archetype;
}

void archetype_delete(World* wld, Archetype* archetype) {
	hashmap_t* chunks = &archetype->chunks;
	entity_t* parent;  Chunk* chunk;
	hashmap_foreach(chunks, parent, chunk)
		chunk_delete(wld, chunk);

	hashmap_free(&archetype->chunks);
	hashmap_remove(&wld->archetype_map, archetype->type);
	event_post(eventhandler_instance(), (Event) { .type = EVT_DELTYPE, .data = archetype->type });
	objectallocator_free(&wld->archetypes, archetype);
}