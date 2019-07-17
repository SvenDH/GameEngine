#include "ecs.h"
#include "systems.h"

static MemoryPool* entity_pool;
static HashMap* entity_map;
static HashMap* entity_component_map;

static HashMap* chunk_map;
static MemoryPool* chunk_pool;

World worlds[MAX_WORLDS];

Entity* entity_new(int world, int x, int y) {
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

int entity_move(Chunk* dest_chunk, Chunk* src_chunk, int dest, int src) {
	assert(dest_chunk->world == src_chunk->world);
	World* world = world_get(dest_chunk->world);
	if (dest >= world->max_ents) return -1;
	uint16_t mask = world->component_mask;
	entity_handle src_comp = { src, src_chunk->id, src_chunk->world };
	entity_handle dest_comp = { dest, dest_chunk->id, dest_chunk->world };
	// Copy all components to new location
	for (int i = 0; i < world->nr_components; i++) {
		memcpy(
			component_get(dest_comp, i), 
			component_get(src_comp, i), 
			world->sizes[i]);
	}
	// Change component handle
	UID* swap_id = component_get(dest_comp, 0);
	hashmap_put(entity_component_map, swap_id, sizeof(UID), dest_comp.ptr);
	return 0;
}

void entity_delete(Entity* ent) {
	entity_handle components;
	hashmap_get(entity_component_map, &ent->id, sizeof(UID), &components);
	// Delete entity data from chunk
	if (components.ptr) {
		Chunk* chunk = world_get(components.world)->chunks[components.chunk];
		chunk->ent_count--;
		//Swap trick
		if (components.index != chunk->ent_count)
			entity_move(chunk, chunk, components.index, chunk->ent_count);
		
		hashmap_remove(entity_component_map, &ent->id, sizeof(UID));
	}
	event_push(EVT_DELENTITY, &ent->id, sizeof(UID));
	hashmap_remove(entity_map, &ent->id, sizeof(UID));
	pool_free(entity_pool, ent);
}

Chunk* chunk_new(int world, int x, int y) {
	World* wld = world_get(world);
	if (wld->chunkcount == MAX_CHUNKS) return NULL;

	Chunk* new_chunk = (Chunk*)pool_alloc(chunk_pool);
	new_chunk->world = world;
	new_chunk->x = x;
	new_chunk->y = y;
	new_chunk->state = CHUNK_LOADING;
	new_chunk->ent_count = 0;

	//Start at 1 to reserve 0 for NULL handle
	for (int i = 0; i < MAX_CHUNKS; i++) {
		if (wld->chunks[i] == NULL) {
			wld->chunks[i] = new_chunk;
			new_chunk->id = i;
			break;
		}
	}
	wld->chunkcount++;

	hashmap_put(chunk_map, new_chunk->key, 12, new_chunk);
	return new_chunk;
}

Chunk* chunk_get(int world, int x, int y) {
	Chunk* chunk;
	uint32_t key[3];
	key[0] = world;
	key[1] = x;
	key[2] = y;
	hashmap_get(chunk_map, key, 12, &chunk);
	return chunk;
}

void* chunk_get_componentarray(Chunk* chunk, int type) {
	World* world = world_get(chunk->world);
	return (void*)(&chunk->data + world->offsets[type]);
}

void* component_get(entity_handle handle, int type) {
	World* world = world_get(handle.world);
	Chunk* chunk = world->chunks[handle.chunk];
	return (void*)((((int)&chunk->data) + world->offsets[type] + handle.index * world->sizes[type]) * (world->component_mask & 1 << type));
}

void chunk_delete(Chunk* chunk) {
	// Delete all entities
	UID* ent_ids = (UID*)&chunk->data;
	for (int i = chunk->ent_count - 1; i >= 0; i--)
		entity_delete(entity_get(&ent_ids[i]));

	World* wld = world_get(chunk->world);
	wld->chunks[chunk->id] = NULL;
	wld->chunkcount--;
	chunk->state = CHUNK_NONE;

	hashmap_remove(chunk_map, chunk->key, 12);
	pool_free(chunk_pool, chunk);
}

World* world_init(int nr_entities, int* sizes, int nr_components) {
	//World id 0 reserved for NULL entities
	static int id = 1;
	assert(id < MAX_WORLDS + 1);

	World* world = &worlds[id-1];
	world->id = id++;
	world->chunkcount = 0;
	world->max_ents = nr_entities;
	world->component_mask = 1;
	world->nr_components = nr_components;

	//First component is entity id
	world->sizes[0] = sizeof(UID);
	world->offsets[0] = 0;
	int offset = world->max_ents * sizeof(UID);
	for (int i = 1; i < nr_components; i++) {
		world->sizes[i] = sizes[i];
		world->offsets[i] = offset;
		offset += world->max_ents * sizes[i];
		world->component_mask = world->component_mask | (sizes[i] > 0) << i;
	}
	//Check if chunk is big enough for entity data
	int chunksize = sizeof(Chunk) - 4 + offset;
	log_info("Size of chunks: %i    Max entities: %i", chunksize, nr_entities);
	assert(chunksize <= CHUNK_SIZE);

	event_push(EVT_NEWWORLD, &world->id, sizeof(int));

	return world;
}

World* world_get(int id) {
	if (id) return &worlds[id-1];
	else return NULL;
}

void world_delete(World* world) {
	for (int i = 1; i < MAX_CHUNKS; i++)
		if (world->chunks[i])
			chunk_delete(world->chunks[i]);
}

//Entity(id)
static int entity_call(lua_State* L) {
	UID* id = luaL_checkstring(L, 1);
	Entity* ent = entity_get(id);
	if (ent) PUSH_LUA_POINTER(L, Entity_mt, ent);
	else lua_pushnil(L); //TODO: should this condition create new entity with this id
	return 1;
}

//Entity:__index("id" / "components")
static int entity_index(lua_State* L) {
	UID* ref = (UID*)luaL_checkudata(L, 1, Entity_mt);
	Entity* ent = entity_get(ref);
	if (ent) {
		int t = lua_type(L, 2);
		if (t == LUA_TSTRING) {
			const char *key = lua_tostring(L, 2);
			entity_handle comps;
			hashmap_get(entity_component_map, &ent->id, sizeof(UID), &comps.ptr);
			if (comps.ptr) {
				int mask = world_get(comps.world)->component_mask;
#define X(name, type) \
				if ((COMPONENT_MASK(name) & mask) && !strcmp(key, #name)) { \
					type* comp = component_get(comps, comp_##name); \
					PUSH_LUA_POINTER(L, #name, comp); \
				} else
ENTITY_DATA
#undef X
				{
					luaL_getmetatable(L, Entity_mt);
					lua_pushvalue(L, 2);
					lua_rawget(L, -2);
				}
				return 1;
			}
			else return luaL_error(L, "error entity does not have components");
		}
		else return luaL_error(L, "error entity cannot be indexed this way");
	}
	else return luaL_error(L, "error entity does not exist");
}

//Entity:__tostring()
static int entity_tostring(lua_State* L) {
	unsigned char* pc = (unsigned char*)luaL_checkudata(L, 1, Entity_mt);
	char buffer[128];
	for (int i = 0; i < sizeof(UID); i++)
		sprintf((char*)(buffer + i * 3), "%02x ", pc[i]);
	lua_pushstring(L, buffer);
	return 1;
}

//Entity:delete()
static int entity_gc(lua_State* L) {
	UID* ref = (UID*)luaL_checkudata(L, 1, Entity_mt);
	Entity* ent = entity_get(ref);
	if (ent) {
		entity_delete(ent);
		return 0;
	}
	else return luaL_error(L, "error entity does not exist");
}

//World{"id", "type", etc...}
static int world_call(lua_State* L) {
	int sizes[MAX_COMPS];
	int total = sizeof(UID);
	if (lua_istable(L, 2)) {
#define X(name, type) \
		if (is_in_table(L, #name, 2)) { \
			sizes[comp_##name] = sizeof(type); \
			total += sizeof(type); \
		} else sizes[comp_##name] = 0;
			ENTITY_DATA
#undef X
		int nr_ents = (CHUNK_SIZE - sizeof(Chunk) + 4) / total;
		World* world = world_init(nr_ents, sizes, NR_COMPS);
		PUSH_LUA_POINTER(L, World_mt, world);
	}
	else luaL_error(L, "error argument should be table");
	return 1;
}

//World:create_entity(x,y)
static int world_create_entity(lua_State* L) {
	World* world = *(World**)luaL_checkudata(L, 1, World_mt);
	//Arguments are the chunk coordinates
	int x = luaL_optinteger(L, 2, 0);
	int y = luaL_optinteger(L, 3, 0);
	//TODO: provide optional id and other components
	Entity* ent = entity_new(world->id, x, y);
	if (ent) {
		UID* ref = lua_newuserdata(L, sizeof(UID));
		luaL_setmetatable(L, Entity_mt);
		memcpy(ref, ent, sizeof(UID));
	}
	else lua_pushnil(L);
	return 1;
}

//World:__gc()
static int world_gc(lua_State* L) {
	World* world = *(World**)luaL_checkudata(L, 1, World_mt);
	world_delete(world);
	return 0;
}

int openlib_ECS(lua_State* L) {
	assert(sizeof(entity_handle) == sizeof(void*));
	assert(NR_COMPS < MAX_COMPS);

	entity_component_map = hashmap_new();

	chunk_pool = pool_init(CHUNK_SIZE, 32);
	chunk_map = hashmap_new();

#define X(name, type) \
	static luaL_Reg name##_func[] = { \
		{ "__index", name##_index }, \
		{ "__newindex", name##_newindex}, \
		{NULL, NULL} }; \
	create_lua_type(L, #name, name##_func);
	ENTITY_DATA
#undef X

	static luaL_Reg ent_func[] = {
		{"delete", entity_gc},
		{"__index", entity_index},
		{"__tostring", entity_tostring},
		{NULL, NULL}
	};
	create_lua_class(L, Entity_mt, entity_call, ent_func);

	static luaL_Reg world_func[] = {
		{"create_entity", world_create_entity},
		{"__gc", world_gc},
		{NULL, NULL}
	};
	create_lua_class(L, World_mt, world_call, world_func);

	system_init(EVT_NEWENTITY, debug_system, 0);
	system_init(EVT_DELENTITY, debug_system, 0);
	//system_init(EVT_DRAW, draw_system, COMP_MASK(transform) | COMP_MASK(sprite));
	return 0;
}