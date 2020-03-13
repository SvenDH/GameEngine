#include "ecs.h"

//TODO: initialize chunks from file
void world_init(world_t* world, const char* name) {
	object_init(&world->chunks, "Ecs.Chunks", CHUNK_SIZE, 16, CHUNK_SIZE);
	object_init(&world->archetypes, "Ecs.Archetypes", sizeof(type_data), 16, 16);
	object_init(&world->systems, "Ecs.Systems", sizeof(system_t), 16, 16); //TODO: remove this and make external systems register

	hashmap_init(&world->entity_map);
	hashmap_init(&world->chunk_map);
	hashmap_init(&world->archetype_map);
	list_init(&world->system_list);
	world->chunk_index = 1;

	event_register(eventhandler_instance(), world,
		addcomponent|setcomponent|delcomponent|addtype|deltype, 
		system_listener, NULL);

	event_register(eventhandler_instance(), world,
		preupdate|update|postupdate|predraw|draw|postdraw, 
		system_update, NULL);

	event_register(eventhandler_instance(), world, 
		addcomponent|setcomponent|delcomponent|addtype|deltype|startcollision|endcollision, 
		ecs_debug_log, NULL);

	//system_new(world, debug_system, 0, collider | transform, disabled, postdraw, NULL);
}

int ecs_addcomponenttype(world_t* world, const char* name, size_t size) {
	assert(ecs_component_counter < sizeof(type_t) * 8);
	int type_id = ecs_component_counter++;
	component_sizes[type_id] = size;

}

int ecs_component_listener(world_t* wld, event_t evt) {
	group_t mask;
	system_t* system;
	chunk_data* chk;
	entity_t ent = (entity_t)evt.p0.data;
	int comp = (int)evt.p1.data;
	switch (evt.type) {	
	case on_delcomponent:
		switch (comp) {
		case comp_container:
			children_foreach(
				wld, 
				((ent_container*)entity_getcomponent(
					wld, ent, comp_container))->children, 
				chk) {
				chunk_change(wld, chk, chk->type, 0);
			}
			break;
		}
	}
	return 0;
}


int ecs_debug_log(world_t* wld, event_t evt) {
	switch (evt.type) {
	case on_addcomponent:
		log_info("Created component of type %i for entity %0llx", (type_t)evt.p0.data, (entity_t)evt.p1.data);
		break;
	case on_setcomponent:
		log_info("Changed component of type %i for entity %0llx", (type_t)evt.p0.data, (entity_t)evt.p1.data);
		break;
	case on_delcomponent:
		log_info("Deleted component of type %i for entity %0llx", (type_t)evt.p0.data, (entity_t)evt.p1.data);
		break;
	case on_addtype:
		log_info("Created type: %i", (type_t)evt.p0.data);
		break;
	case on_deltype:
		log_info("Deleted type: %i", (type_t)evt.p0.data);
		break;
	case on_startcollision:
		log_info("Collision started between entity %0llx and entity %0llx", (entity_t)evt.p0.data, (entity_t)evt.p1.data);
		break;
	case on_endcollision:
		log_info("Collision ended between entity %0llx and entity %0llx", (entity_t)evt.p0.data, (entity_t)evt.p1.data);
		break;
	}
	return 0;
}


//Entity(entity / id / table [, parent])
static int w_entity_call(lua_State* L) {
	const char* str;
	world_t* wld = world_instance();
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
#define X(_name, _size, attr) \
		lua_pushstring(L, #_name); \
		lua_gettable(L, 2); \
		if (!lua_isnil(L, -1)) \
			components |= _name; \
		lua_pop(L, 1);
		ENTITY_DATA
#undef X
			ent = entity_new(wld, 0, components, parent);
		if (ent) {
			int index = 0, comp_index = 0;
			chunk_data* chunk = entity_to_chunk(wld, ent, &index);

			entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;

			set_component_from_table(L, 2);
			return 1;
		}
		break;
	case LUA_TSTRING:
		str = lua_tostring(L, 2);
		entity_t ref = strtol(str, NULL, sizeof(entity_t)); //TODO: fix this
		ent = *entity_get(wld, ref);
		if (ent) {
			entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;
			return 1;
		}
		break;
	case LUA_TUSERDATA:
		ref = (entity_t*)luaL_checkudata(L, 2, Entity_mt);
		//TODO: copy entity ref to new entity
		break;

	}
	lua_pushnil(L);
	return 1;
}

//Entity:__index("id" / "components")
static int w_entity_index(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	entity_t ent = *ref;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		chunk_data* chunk = entity_to_chunk(wld, ent, &index);
		if (chunk) {
			type_t mask = chunk->type;
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				if (chunk->parent) {
					entity_t* refpar = lua_newuserdata(L, sizeof(entity_t));
					luaL_setmetatable(L, Entity_mt);
					*refpar = chunk->parent;
				}
				else lua_pushnil(L);
			} else
#undef KEY
#define KEY "type"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				lua_pushinteger(L, mask);
			}
			else
#undef KEY
#define KEY "chunk"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				PUSH_LUA_POINTER(L, Chunk_mt, chunk);
			}
			else
#undef KEY
#define X(name, _size, attr) \
			if (len == sizeof(#name)-1 && !memcmp(key, #name, sizeof(#name)-1)) { \
				if (!(name & mask)) { \
					entity_changetype(wld, ent, mask | name); \
					chunk = entity_to_chunk(wld, *ref, &index); \
				} \
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
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	entity_t ent = *ref;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		chunk_data* chunk = entity_to_chunk(wld, ent, &index);
		if (chunk) {
			type_t mask = chunk->type;
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				entity_t new_parent = chunk->parent;
				if (lua_type(L, 3) == LUA_TNIL)
					new_parent = 0;
				else
					new_parent = *(entity_t*)luaL_checkudata(L, 3, Entity_mt);
				entity_changeparent(wld, ent, new_parent);
			} else
#undef KEY
#define KEY "type"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				type_t new_type = (type_t)luaL_checkinteger(L, 3);
				entity_changetype(wld, ent, new_type);
			}
			else
#undef KEY
#define X(_name, _size, _attr) \
			if (len == sizeof(#_name)-1 && !memcmp(key, #_name, sizeof(#_name)-1)) { \
				if (!(_name & mask)) { \
					entity_changetype(wld, ent, mask | _name); \
					chunk = entity_to_chunk(wld, ent, &index); \
				} \
				void* comp = chunk_get_component(chunk, comp_##_name, index); \
				PUSH_LUA_POINTER(L, #_name, comp); \
				set_component_from_table(L, 3); \
			} else
			ENTITY_DATA
#undef X
			{
				return luaL_error(L, "error entity does not have this attribute");
			}
			return 0;
		}
		else return luaL_error(L, "error entity does not exist");
	}
	else return luaL_error(L, "error entity cannot be indexed this way");
}

//Entity:add_component()
static int w_entity_add(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | (1 << comp_id))) == 0);
	return 1;
}

//Entity:remove_component()
static int w_entity_remove(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~(1 << comp_id))) == 0);
	return 1;
}

//Entity:enable()
static int w_entity_enable(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~(1 << comp_disabled))) == 0);
	return 1;
}

//Entity:disable()
static int w_entity_disable(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | 1 << comp_disabled)) == 0);
	return 1;
}

//Entity:__tostring()
static int w_entity_tostring(lua_State* L) {
	entity_t* ent = luaL_checkudata(L, 1, Entity_mt);
	char buff[17];
	sprintf(buff, "%0llx", *ent);
	lua_pushlstring(L, buff, 16);
	return 1;
}

//Entity:delete()
static int w_entity_gc(lua_State* L) {
	entity_t* ref = luaL_checkudata(L, 1, Entity_mt);
	entity_delete(world_instance(), *ref);
	return 0;
}

static int w_chunk_entities_iter(lua_State* L) {
	chunk_data* chk = *(chunk_data**)lua_touserdata(L, lua_upvalueindex(1));
	uint16_t index = (uint16_t)lua_tointeger(L, lua_upvalueindex(2));
	if (index < chk->ent_count) {
		entity_t* ent = (entity_t*)chunk_get_component(chk, comp_id, index);
		entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
		luaL_setmetatable(L, Entity_mt);
		*ref = *ent;
		lua_pushnumber(L, ++index);
		lua_replace(L, lua_upvalueindex(2));
		return 1;
	}
	return 0;
}

static int w_chunk_index(lua_State* L) {
	chunk_data* chk = luaL_checkudata(L, 1, Chunk_mt);
	size_t len;
	const char *key;
	switch (lua_type(L, 2)) {
	case LUA_TSTRING:
		key = lua_tolstring(L, 2, &len);
#define KEY "entities"
		if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
			lua_pushvalue(L, 1);
			lua_pushinteger(L, 0);
			lua_pushcclosure(L, w_chunk_entities_iter, 2);
		}
		else
#undef KEY
			lua_pushnil(L);
		break;
	}
	return 1;
}

static int w_chunk_gc(lua_State* L) {
	chunk_data* chk = luaL_checkudata(L, 1, Chunk_mt);
	chunk_delete(world_instance(), chk);
	return 0;
}

int openlib_ECS(lua_State* L) {
	assert(NR_COMPS < MAX_COMPS);
	//TODO: add more functions to components
#define X(name, type, size, attr) \
	static luaL_Reg name##_func[] = { \
		{ "__index", w_##name##_index }, \
		{ "__newindex", w_##name##_newindex}, \
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
#define X(name, type, size, attr) \
	lua_pushlstring(L, #name, sizeof(#name) - 1); \
	lua_pushnumber(L, comp_##name); \
	lua_settable(L, -3);
	ENTITY_DATA
#undef X
	lua_setglobal(L, "component");

	static luaL_Reg chunk_func[] = {
		{"delete", w_chunk_gc},
		{"__index", w_chunk_index},
		{NULL, NULL}
	};
	create_lua_type(L, Chunk_mt, chunk_func);
	
	return 0;
}
