#include "ecs.h"
#include "math.h"


chunk_data* component_to_chunk(void* comp_ptr, int comp_type, int* index) {
	intptr_t loc = (intptr_t)comp_ptr; //Find chunk using allignment of chunk
	chunk_data* chk = (chunk_data*)(loc - (loc % CHUNK_SIZE));
	if (index) {
		intptr_t arr = (intptr_t)chunk_get_componentarray(chk, comp_type);
		*index = (loc - arr) / component_sizes[comp_type];
	}
	return chk;
}

static int child_iter(lua_State* L) {
	chunk_t child_index = (chunk_t)lua_tointeger(L, lua_upvalueindex(1));
	if (child_index) {
		int index = (int)lua_tointeger(L, lua_upvalueindex(2));
		chunk_data* chk = hashmap_get(&world_instance()->chunk_map, child_index);
		entity_t* ent = chunk_get_component(chk, comp_id, index);
		entity_t* ref = lua_newuserdata(L, sizeof(entity_t));
		luaL_setmetatable(L, Entity_mt);
		*ref = *ent;
		if (++index == chk->ent_count) {
			lua_pushnumber(L, chk->next_children);
			lua_replace(L, lua_upvalueindex(1));
			index = 0;
		}
		lua_pushnumber(L, index);
		lua_replace(L, lua_upvalueindex(2));
		return 1;
	}
	return 0;
}

inline void get_int(lua_State* L, int32_t integer) {
	lua_pushinteger(L, integer);
}

inline void set_int(lua_State* L, int32_t* integer, int i) {
	*integer = (int32_t)luaL_checkinteger(L, i);
}

inline void get_short(lua_State* L, int16_t integer) {
	lua_pushinteger(L, integer);
}

inline void set_short(lua_State* L, int16_t* integer, int i) {
	*integer = (int16_t)luaL_checkinteger(L, i);
}

inline void get_float(lua_State* L, float num) {
	lua_pushnumber(L, num);
}

inline void set_float(lua_State* L, float* num, int i) {
	*num = (float)luaL_checknumber(L, i);
}

inline void get_vec2(lua_State* L, vec2 vector) {
	vec2* vec = lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*vec)[0] = vector[0];
	(*vec)[1] = vector[1];
}

inline void set_vec2(lua_State* L, vec2* vector, int i) {
	vec2* vec = luaL_checkudata(L, i, Vector_mt);
	(*vector)[0] = (*vec)[0];
	(*vector)[1] = (*vec)[1];
}

inline void get_rid_t(lua_State* L, rid_t resource) {
	*(rid_t*)lua_newuserdata(L, sizeof(rid_t)) = resource;
	luaL_setmetatable(L, res_mt[resource.type]);
}

inline void set_rid_t(lua_State* L, rid_t* resource, int i) {
	*resource = *(rid_t*)luaL_checkudata(L, i, res_mt[resource->type]);
	//Make sure resource is loaded
	lua_getfield(L, i, "load");
	lua_pushvalue(L, i);
	lua_call(L, 1, 0);
}

inline void get_entity_t(lua_State* L, entity_t id) {
	entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
	luaL_setmetatable(L, Entity_mt);
	*ref = id;
}

inline void set_entity_t(lua_State* L, entity_t* id, int i) {
	entity_t* ref = luaL_checkudata(L, i, Entity_mt);
	*id = *ref;
}

inline void get_byte(lua_State* L, byte* array) {
	//Created a 2d array
	lua_newtable(L);
	for (int y = 0; y < TILE_ROWS; y++) {
		lua_newtable(L);
		for (int x = 0; x < TILE_COLS; x++) {
			lua_pushinteger(L, array[y * TILE_ROWS + x]);
			lua_rawseti(L, -2, x + 1);
		}
		lua_rawseti(L, -2, y + 1);
	}
}

inline void set_byte(lua_State* L, byte* array, int i) {
	const char* str;
	size_t len;
	switch (lua_type(L, i)) {
	case LUA_TTABLE:
		//Assume 2d array as lua table
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			lua_pushvalue(L, -2);
			int y_index = (int)lua_tointeger(L, -1);
			luaL_checktype(L, -2, LUA_TTABLE);
			lua_pushvalue(L, -2);
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				lua_pushvalue(L, -2);
				int x_index = (int)lua_tointeger(L, -1);
				char value = (char)lua_tointeger(L, -2);
				array[(y_index - 1) * TILE_ROWS + (x_index - 1)] = value;
				lua_pop(L, 2);
			}
			lua_pop(L, 3);
		}
		break;
	case LUA_TSTRING:
		str = lua_tolstring(L, i, &len);
		memcpy(array, str, len);
	}
}

inline void get_chunk_t(lua_State* L, chunk_t child_index) {
	world_t* wld = world_instance();
	lua_pushinteger(L, child_index);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, child_iter, 2);
}

inline void set_chunk_t(lua_State* L, chunk_t* child_index, int i) {
	//set_chunk_t function relies on chunk_t attribute being the first
	luaL_checktype(L, i, LUA_TTABLE);
	world_t* wld = world_instance();
	chunk_data* chk = NULL;
	//First remove all children
	children_foreach(wld, *child_index, chk)
		chunk_change(wld, chk, chk->type, 0);
	int index;
	chk = component_to_chunk(child_index, comp_container, &index);
	entity_t* parent = chunk_get_component(chk, comp_id, index);
	//Then add children from array (assume there is a lua table at index i)
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		entity_t* ref = luaL_checkudata(L, -2, Entity_mt);
		entity_changeparent(wld, *ref, *parent);
		lua_pop(L, 2);
	}
}

//Generate getters and setters for all components 
#define Y(_type, _val) \
		if (len == sizeof(#_val)-1 && !memcmp(key, #_val, sizeof(#_val)-1)) \
			get_##_type(L, comp->_val); \
		else
#define X(_name, _size, _attr) \
int w_##_name##_index(lua_State *L) { \
	if (_size) { /* Use compiler optimization to skip this test for constant */\
		ent_##_name* comp = *(void**)lua_touserdata(L, 1); \
		int t = lua_type(L, 2); \
		if (t == LUA_TSTRING) { \
			size_t len; \
			const char *key = lua_tolstring(L, 2, &len); \
			_attr \
			lua_pushnil(L); \
		} \
		else lua_pushnil(L); \
		return 1; \
	} else return 0; \
}
ENTITY_DATA
#undef X
#undef Y

#define Y(_type, _val) \
		if (len == sizeof(#_val)-1 && !memcmp(key, #_val, sizeof(#_val)-1)) \
			set_##_type(L, &comp->_val, 3); \
		else
#define X(_name, _size, _attr) \
int w_##_name##_newindex(lua_State *L) { \
	if (_size) { /* Use compiler optimization to skip this test for constant */ \
		ent_##_name* comp = *(void**)lua_touserdata(L, 1); \
		int t = lua_type(L, 2); \
		if (t == LUA_TSTRING) { \
			size_t len; \
			const char *key = lua_tolstring(L, 2, &len); \
			_attr \
			/*else*/return 0; \
			int index; \
			chunk_data* chk = component_to_chunk(comp, comp_##_name, &index); \
			entity_t* ent = chunk_get_component(chk, comp_id, index); \
			event_post(eventhandler_instance(), \
				(event_t) {.type = on_setcomponent,.p0.data = comp_##_name,.p1.data = *ent }); \
		} \
	} \
	return 0; \
}
ENTITY_DATA
#undef X
#undef Y
