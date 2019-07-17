#include "entity.h"

static MemoryPool* entity_pool;
static HashMap* entity_map;

Entity* entity_new(int world, int x, int y) {
	if (!entity_map) {
		entity_pool = pool_init(sizeof(Entity), 1024);
		entity_map = hashmap_new();
	}

	Chunk* chunk = chunk_get(world, x, y);
	if (!chunk) chunk = chunk_new(world, x, y);
	if (chunk->ent_count == world_get(chunk->world)->max_ents) return NULL;

	Entity* new_entity = pool_alloc(entity_pool);
	//TODO: check and use input id
	random_uuid(&new_entity->id);
	hashmap_put(entity_map, &new_entity->id, sizeof(UID), new_entity);

	entity_handle new_components;
	new_components.world = world;
	new_components.chunk = chunk->id;
	new_components.index = chunk->ent_count++;
	hashmap_put(entity_component_map, &new_entity->id, sizeof(UID), new_components.ptr);

	UID* eid = component_get(new_components, 0);
	memcpy(eid, &new_entity->id, sizeof(UID));
	event_push(EVT_NEWENTITY, &new_entity->id, sizeof(UID));
	return new_entity;
}

Entity* entity_get(UID* id) {
	Entity* ent;
	hashmap_get(entity_map, id, sizeof(UID), &ent);
	return ent;
}