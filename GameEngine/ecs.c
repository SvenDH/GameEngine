#include "ecs.h"

//TODO: initialize chunks from file
World* world_instance() {
	static World* world = NULL;
	if (!world) {
		world = (World*)malloc(sizeof(World));
		objectallocator_init(&world->chunks, "Ecs.Chunks", CHUNK_SIZE, 16, CHUNK_SIZE);
		objectallocator_init(&world->archetypes, "Ecs.Archetypes", sizeof(Archetype), 16, 16);
		objectallocator_init(&world->systems, "Ecs.Systems", sizeof(System), 16, 16);

		hashmap_init(&world->entity_map);
		hashmap_init(&world->chunk_map);
		hashmap_init(&world->archetype_map);

		world->chunk_index = 1;
	}
	return world;
}

//Entity(entity / id / table [, parent])
static int w_entity_call(lua_State* L) {
	char* str;
	World* wld = world_instance();
	entity_t ent = 0;
	entity_t* ref;
	type_t components = 0;

	entity_t parent = 0;
	if (lua_gettop(L) > 2) {
		entity_t* parent_ref = luaL_checkudata(L, 3, Entity_mt);
		if (parent_ref) parent = *parent_ref;
	}

	switch (lua_type(L, 2)) {
	case LUA_TTABLE:
#define X(name, type) \
		lua_pushstring(L, #name); \
		lua_gettable(L, 2); \
		if (!lua_isnil(L, -1)) \
			components |= name; \
		lua_pop(L, 1);
		ENTITY_DATA
#undef X
			ent = entity_new(wld, 0, components, parent);
		if (ent) {
			int index, comp_index = 0;
			Chunk* chunk = entity_to_chunk(wld, ent, &index);

			entity_t* ref = lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;

			set_component_from_table(L, 2);
			return 1;
		}
		break;
	case LUA_TSTRING:
		str = lua_tostring(L, 2);
		entity_t ref = strtol(str, NULL, 16); //TODO: fix this
		ent = *entity_get(wld, ref);
		if (ent) {
			entity_t* ref = lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;
			return 1;
		}
		break;
	case LUA_TUSERDATA:
		ref = luaL_checkudata(L, 2, Entity_mt);
		//TODO: copy entity ref to new entity
		break;

	}
	lua_pushnil(L);
	return 1;
}

//Entity:__index("id" / "components")
static int w_entity_index(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		Chunk* chunk = entity_to_chunk(wld, *ref, &index);
		if (chunk) {
			type_t mask = chunk->type; //TODO: make this a function
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				if (chunk->parent) {
					entity_t* ref = lua_newuserdata(L, sizeof(entity_t));
					luaL_setmetatable(L, Entity_mt);
					*ref = chunk->parent;
				}
				else lua_pushnil(L);
			} else
#undef KEY
#define X(name, type) \
			if ((name & mask) && len == sizeof(#name)-1 && !memcmp(key, #name, sizeof(#name)-1)) { \
				void* comp = chunk_get_component(chunk, comp_##name, index); \
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
		else return luaL_error(L, "error entity does not exist");
	}
	else return luaL_error(L, "error entity cannot be indexed this way");
}

//Entity:__newindex("id" / "components")
static int w_entity_newindex(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		Chunk* chunk = entity_to_chunk(wld, *ref, &index);
		if (chunk) {
			type_t mask = chunk->type;
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				entity_t* new_parent = luaL_checkudata(L, 3, Entity_mt);
				entity_changeparent(wld, *ref, *new_parent);
			} else
#undef KEY
#define X(name, type) \
			if ((name & mask) && len == sizeof(#name)-1 && !memcmp(key, #name, sizeof(#name)-1)) { \
				void* comp = chunk_get_component(chunk, comp_##name, index); \
				PUSH_LUA_POINTER(L, #name, comp); \
				set_component_from_table(L, 3); \
			} else
			ENTITY_DATA
#undef X
			{
				return luaL_error(L, "error entity does not have this component");
			}
			return 0;
		}
		else return luaL_error(L, "error entity does not exist");
	}
	else return luaL_error(L, "error entity cannot be indexed this way");
}

//Entity:add_component()
static int w_entity_add(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | (1 << comp_id))) == 0);
	return 1;
}

//Entity:remove_component()
static int w_entity_remove(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~(1 << comp_id))) == 0);
	return 1;
}

//Entity:enable()
static int w_entity_enable(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~(1 << comp_disabled))) == 0);
	return 1;
}

//Entity:disable()
static int w_entity_disable(lua_State* L) {
	World* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | 1 << comp_disabled)) == 0);
	return 1;
}

//Entity:__tostring()
static int w_entity_tostring(lua_State* L) {
	entity_t* ent = luaL_checkudata(L, 1, Entity_mt);
	char buff[16];
	sprintf(buff, "0x%llx", *ent);
	lua_pushlstring(L, buff, 16);
	return 1;
}

//Entity:delete()
static int w_entity_gc(lua_State* L) {
	entity_t* ref = luaL_checkudata(L, 1, Entity_mt);
	entity_delete(world_instance(), *ref);
	return 0;
}

int openlib_ECS(lua_State* L) {
	assert(NR_COMPS < MAX_COMPS);

#define X(name, type) \
	static luaL_Reg name##_func[] = { \
		{ "__index", name##_index }, \
		{ "__newindex", name##_newindex}, \
		{NULL, NULL} }; \
	create_lua_type(L, #name, name##_func);
	ENTITY_DATA
#undef X

	static luaL_Reg ent_func[] = {
		{"delete", w_entity_gc},
		{"enable", w_entity_enable},
		{"disable", w_entity_disable},
		{"add_component", w_entity_add},
		{"remove_component", w_entity_remove},
		{"__index", w_entity_index},
		{"__newindex", w_entity_newindex},
		{"__tostring", w_entity_tostring},
		{NULL, NULL}
	};
	create_lua_class(L, Entity_mt, w_entity_call, ent_func);

	lua_newtable(L);
#define X(name) \
	lua_pushlstring(L, #name, sizeof(#name) - 1); \
	lua_pushnumber(L, comp_##name); \
	lua_settable(L, -3);
	ENTITY_DATA
#undef X
	lua_setglobal(L, "component");

	system_new(world_instance(), draw_system, transform | sprite | id, disabled, EVT_DRAW);
	system_new(world_instance(), move_system, transform | physics | id, disabled, EVT_UPDATE);
	//system_new(world_instance(), debug_system, 0, 0, 4, EVT_NEWENTITY, EVT_DELENTITY, EVT_UPDATE, EVT_GUI);
	return 0;
}
