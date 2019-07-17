#include "game.h"

//Move entity data from source chunk to destination chunk
int chunk_move_entity(Chunk* dest_chunk, Chunk* src_chunk, int dest, int src) {
	assert(dest_chunk->world == src_chunk->world);
	World* world = &worlds[dest_chunk->world];
	if (dest < world->max_ents) return -1;
	int mask = world->component_mask;
	// Copy all components to new location
	int offset;
#define X(name, type) if (COMPONENT_MASK(name) & mask) \
		memcpy(COMPONENT_OFFSET(dest_chunk, name) + dest * sizeof(type), COMPONENT_OFFSET(src_chunk, name) + src * sizeof(type), sizeof(type));
	ENTITY_DATA
#undef X
	return 0;
}

//Entity is add to end of chunk (uninitialized)
int chunk_add_entity(Chunk* chunk, Entity* entity) {
	if (chunk->ent_count == worlds[chunk->world].max_ents) return -1;
	entity->chunk = chunk;
	entity->index = chunk->ent_count++;
	//TODO: keep entity order by recursively moving last entity
	return 0;
}

//Move last entity (if there is still an entity) to new slot in chunk
int chunk_remove_entity(Chunk* chunk, Entity* entity) {
	if (entity->chunk != chunk) return -1;
	entity->chunk = NULL;
	chunk->ent_count--;
	if (chunk->ent_count && entity->index != chunk->ent_count)
		chunk_move_entity(chunk, chunk, entity->index, chunk->ent_count);
	//TODO: keep entity order by recursively moving last entity
	return 0;
}

//World({"id", "type", etc...})
static int world_init(lua_State* L) {
	static int id = 0;
	assert(id < MAX_WORLDS);

	World* world = &worlds[id];
	world->id = id++;
	world->chunkcount = 0;
	world->max_ents = MAX_ENT;	
	world->component_mask = 0;
	world->list = NULL;

	int offset = 0;
#define X(name, type) \
	if (is_in_table(L, #name, 2)) { \
		world->sizes[comp_##name] = sizeof(type); \
		world->offsets[comp_##name] = offset; \
		offset += world->max_ents * sizeof(type); \
		world->component_mask = world->component_mask | COMPONENT_MASK(name); } \
	else { \
		world->sizes[comp_##name] = 0; \
		world->offsets[comp_##name] = -1; \
	}
	ENTITY_DATA
#undef X

	//Check if chunk is big enough for entity data
	int chunksize = sizeof(Chunk) - 4 + offset;
	printf("Size of chunks: %i\n", chunksize);
	assert(chunksize <= CHUNK_SIZE);

	PUSH_LUA_POINTER(L, World_mt, world);
	return 1;
}

Chunk* chunk_new(World* world, int x, int y) {
	Chunk* chunk = (Chunk*)pool_alloc(chunks);
	//memset(chunk, 0, sizeof(Chunk));
	chunk->world = world->id;
	chunk->x = x;
	chunk->y = y;
	hashmap_put(chunk_map, chunk->key, 12, chunk);
	chunk->state = CHUNK_LOADING;
	chunk->ent_count = 0;
	//Insert chunk into world chunk list
	if (world->list) {
		chunk->next = world->list;
		world->list->prev = chunk;
	}
	else chunk->next = NULL;
	world->list = chunk;
	chunk->component_mask = world->component_mask;
	chunk->offsets = world->offsets;

	return chunk;
}

void chunk_delete(Chunk* chunk) {
	//Remove chunk from list and relink
	if (chunk->prev)
		chunk->prev->next = chunk->next;
	if (chunk->next)
		chunk->next->prev = chunk->prev;
	chunk->state = CHUNK_NONE;
	hashmap_remove(chunk_map, chunk->key, 8);
	pool_free(chunks, chunk);
	return 0;
}

Chunk* chunk_find(World* world, int x, int y) {
	Chunk* chunk;
	uint32_t key[3];
	key[0] = world->id;
	key[1] = x;
	key[2] = y;
	hashmap_get(chunk_map, key, 12, &chunk);
	return chunk;
}

int openlib_Game(lua_State* L) {
	static luaL_Reg world_func[] = {
		{"new_chunk", chunk_new},
		{"get_chunks", chunk_iter},
		{"find", chunk_find},
		{NULL, NULL}
	};

	create_lua_class(L, World_mt, world_init, world_func);

	return 0;
}

/*
HashMap* entity_map;
MemoryPool* entities;

HashMap* chunk_map;
MemoryPool* chunks;

World worlds[MAX_WORLDS];

static int chunk_next(lua_State *L);

//Move entity data from source chunk to destination chunk
int chunk_move_game_object(Chunk* dest_chunk, Chunk* src_chunk, int dest, int src) {
	assert(dest_chunk->world == src_chunk->world);
	World* world = &worlds[dest_chunk->world];
	if (dest < world->max_ents) return -1;
	int mask = world->component_mask;
	// Copy all components to new location
	int offset;
#define X(name, type) if (COMPONENT_MASK(name) & mask) \
		memcpy(COMPONENT_OFFSET(dest_chunk, name) + dest * sizeof(type), COMPONENT_OFFSET(src_chunk, name) + src * sizeof(type), sizeof(type));
	ENTITY_DATA
#undef X
	// Change entity pointers
	Entity* ent;
	ent_id* id = ENT_GET_COMPONENT(dest_chunk, id, dest);
	hashmap_get(entity_map, (char*)id, sizeof(ent_id), &ent);
	assert(ent != NULL);
	ent->chunk = dest_chunk;
	ent->index = dest;
	return 0;
}

//Entity is add to end of chunk (uninitialized)
int chunk_add_game_object(Chunk* chunk, Entity* entity) {
	if (chunk->ent_count == worlds[chunk->world].max_ents) return -1;
	entity->chunk = chunk;
	entity->index = chunk->ent_count++;
	//TODO: keep entity order by recursively moving last entity
	return 0;
}

//Move last entity (if there is still an entity) to new slot in chunk
int chunk_remove_game_object(Chunk* chunk, Entity* entity) {
	if (entity->chunk != chunk) return -1;
	entity->chunk = NULL;
	chunk->ent_count--;
	if (chunk->ent_count && entity->index != chunk->ent_count)
		chunk_move_game_object(chunk, chunk, entity->index, chunk->ent_count);
	//TODO: keep entity order by recursively moving last entity
	return 0;
}

// Entity([id])
static int entity_create(lua_State* L) {
	Entity* new_entity = pool_alloc(entities);
	//TODO: check and use input id
	random_uuid(&new_entity->id);
	hashmap_put(entity_map, &new_entity->id, sizeof(ent_id), new_entity);
	new_entity->chunk = NULL;
	PUSH_LUA_POINTER(L, Entity_mt, new_entity);
	return 1;
}

static int entity_newindex(lua_State* L) {
	Entity* ent = (Entity*)luaL_checkudata(L, 1, Entity_mt);
	Chunk* chunk = ent->chunk;
	if (chunk == NULL) return luaL_error(L, "Error: Entity should have chunk");
	int mask = worlds[chunk->world].component_mask;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
#define X(name, type) \
		if ((COMPONENT_MASK(name) & mask) && !strcmp(key, #name)) { \
			type* new_val = luaL_checkudata(L, 3, #name); \
			memcpy(ENT_GET_COMPONENT(chunk, name, ent->index), new_val, sizeof(type)); \
			return 0; \
		} else
		ENTITY_DATA
#undef X
		return luaL_error(L, "Error: Entity does not have this field");
	}
	return luaL_error(L, "Error: Entity cannot be indexed");
}

//Entity:delete()
static int entity_delete(lua_State* L) {
	Entity* ent = (Entity*)luaL_checkudata(L, 1, Entity_mt);
	Chunk* chunk;
	int index, i;
	// Loop over all worlds and delete entity data
	if (ent->chunk) {
		chunk = ent->chunk;
		worlds[chunk->world].remove_func(chunk, ent);
	}
	hashmap_remove(entity_map, &ent->id, sizeof(ent_id));
	pool_free(entities, ent);
	return 0;
}

//Entity["id"]
static int entity_index(lua_State* L) {
	Entity *ent = (Entity*)luaL_checkudata(L, 1, Entity_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "id")) {
			lua_pushlstring(L, ent->id, 16);
		}
	}
	else lua_pushnil(L);
	return 1;
}

//Chunk(world, x, y)
static int chunk_new(lua_State* L) {
	World* world = *(World**)luaL_checkudata(L, 2, World_mt);
	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);

	Chunk* chunk = (Chunk*)pool_alloc(chunks);
	//memset(chunk, 0, sizeof(Chunk));
	chunk->world = world->id;
	chunk->x = x;
	chunk->y = y;
	hashmap_put(chunk_map, chunk->key, 12, chunk);
	chunk->state = CHUNK_LOADING;
	chunk->ent_count = 0;
	//Insert chunk into world chunk list
	if (world->list) {
		chunk->next = world->list;
		world->list->prev = chunk;
	}
	else chunk->next = NULL;
	world->list = chunk;

	PUSH_LUA_POINTER(L, Chunk_mt, chunk);
	return 1;
}

//Chunk["x"/"y"/"world"]
static int chunk_index(lua_State* L) {
	Chunk *chunk = *(Chunk**)luaL_checkudata(L, 1, Chunk_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "x")) lua_pushinteger(L, chunk->x);
		else if (!strcmp(key, "y")) lua_pushinteger(L, chunk->y);
		else if (!strcmp(key, "world")) {
			PUSH_LUA_POINTER(L, World_mt, &worlds[chunk->world]);
		}
		else {
			//TODO: make common class for all objects
			lua_getmetatable(L, 1);
			lua_pushvalue(L, 2);
			lua_gettable(L, -2);
		}
	}
	else lua_pushnil(L);
	return 1;
}

//Chunk:add(entity)
static int chunk_add_entity(lua_State* L) {
	Chunk* chunk = (Chunk*)luaL_checkudata(L, 1, Chunk_mt);
	Entity* ent = (Entity*)luaL_checkudata(L, 2, Entity_mt);
	if (worlds[chunk->world].add_func(chunk, ent))
		return luaL_error(L, "Error: Chunk is full");
	//TODO: copy entity
	return 0;
}

//Chunk:remove(entity)
static int chunk_remove_entity(lua_State* L) {
	Chunk* chunk = *(Chunk**)luaL_checkudata(L, 1, Chunk_mt);
	Entity* ent = *(Entity**)luaL_checkudata(L, 2, Entity_mt);
	if (worlds[chunk->world].remove_func(chunk, ent))
		return luaL_error(L, "Error: Entity not in chunk");
	return 0;
}

//Chunk:delete()
static int chunk_delete(lua_State* L) {
	Chunk* chunk = *(Chunk**)luaL_checkudata(L, 1, Chunk_mt);
	//Remove chunk from list and relink
	if (chunk->prev)
		chunk->prev->next = chunk->next;
	if (chunk->next)
		chunk->next->prev = chunk->prev;
	chunk->state = CHUNK_NONE;
	hashmap_remove(chunk_map, chunk->key, 8);
	pool_free(chunks, chunk);
	return 0;
}

//World({"id", "type", etc...})
static int world_init(lua_State* L) {
	static int id = 0;
	assert(id < MAX_WORLDS);

	World* world = &worlds[id];
	world->id = id++;
	world->chunkcount = 0;
	world->max_ents = MAX_ENT;
	world->component_mask = 0;
	world->list = NULL;

	int offset = 0;
#define X(name, type) if (is_in_table(L, #name, 2)) { \
	world->offsets[comp_##name] = offset; \
	offset += world->max_ents * sizeof(type); \
	world->component_mask = world->component_mask | COMPONENT_MASK(name); } \
	else { world->offsets[comp_##name] = -1; }
	ENTITY_DATA
#undef X

	world->add_func = chunk_add_game_object;
	world->remove_func = chunk_remove_game_object;
	world->move_func = chunk_move_game_object;

	//Check if chunk is big enough for entity data
	int chunksize = sizeof(Chunk) - 4 + offset;
	printf("Size of chunks: %i\n", chunksize);
	assert(chunksize <= CHUNK_SIZE);

	PUSH_LUA_POINTER(L, World_mt, world);
	return 1;
}

//World:get_chunks()
static int chunk_iter(lua_State *L) {
	World* world = *(World**)luaL_checkudata(L, 1, World_mt);
	world->next = world->list;
	lua_pushcclosure(L, chunk_next, 1);
	return 1;
}

static int chunk_next(lua_State *L) {
	World* world = *(World**)lua_touserdata(L, lua_upvalueindex(1));
	if (world->next) {
		PUSH_LUA_POINTER(L, Chunk_mt, world->next);
		world->next = world->next->next;
		return 1;
	}
	else return 0;
}

//World:find(x, y)
static int chunk_find(lua_State* L) {
	int x, y;
	x = luaL_checkinteger(L, 2);
	y = luaL_checkinteger(L, 3);
	World* world = *(World**)luaL_checkudata(L, 1, World_mt);
	uint32_t key[3];
	key[0] = world->id;
	key[1] = x;
	key[2] = y;
	Chunk* chunk;
	if (hashmap_get(chunk_map, key, 12, &chunk))
		lua_pushnil(L);
	else
		PUSH_LUA_POINTER(L, Chunk_mt, chunk);
	//TODO: check if chunk is saved in file
	return 1;
}

int openlib_Game(lua_State* L) {
	static luaL_Reg ent_func[] = {
		{"__index", entity_index},
		{"__newindex", entity_newindex},
		{"delete", entity_delete},
		{NULL, NULL}
	};

	static luaL_Reg chunk_func[] = {
		{"__index", chunk_index},
		{"add", chunk_add_entity},
		{"remove", chunk_remove_entity},
		{"delete", chunk_delete},
		{NULL, NULL}
	};

	static luaL_Reg world_func[] = {
		{"new_chunk", chunk_new},
		{"get_chunks", chunk_iter},
		{"find", chunk_find},
		{NULL, NULL}
	};

	entities = pool_init(sizeof(Entity), MAX_ENT);
	entity_map = hashmap_new();

	chunks = pool_init(CHUNK_SIZE, 32);
	chunk_map = hashmap_new();

	create_lua_class(L, Entity_mt, entity_create, ent_func);
	create_lua_class(L, Chunk_mt, chunk_new, chunk_func);
	create_lua_class(L, World_mt, world_init, world_func);

	return 0;
}
*/

/*
void chunk_tiles_draw(Chunk* chunk) {
	sprite_draw_tiles(
		chunk->tile_data.tile_tex,
		&chunk->tile_data.type,
		chunk->x * TW * RW * CW,
		chunk->y * TH * RH * CH,
		RW * CW, RH * CH
	);
}

void chunk_entities_draw(Chunk* chunk) {
	int i;
	int x, y;
	ent_transform* pos = ENT_GET_COMPONENT(chunk, transform, 0);
	ent_sprite* spr = ENT_GET_COMPONENT(chunk, sprite, 0);

	for (i = 0; i < chunk->ent_count; i++) {
		sprite_draw_quad(
			spr[i].tex,
			spr[i].index,
			pos[i].position[0] + spr[i].offset[0],
			pos[i].position[1] + spr[i].offset[1],
			spr[i].tex->width, spr[i].tex->height,
			0, 0, 1, 1, 0xFFFFFF, 1
		);
	}
}
*/