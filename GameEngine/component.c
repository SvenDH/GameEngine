#include "ecs.h"
#include "component.h"

inline void get_integer(lua_State* L, int* integer) {
	lua_pushinteger(L, *integer);
}

inline void set_integer(lua_State* L, int* integer, int i) {
	*integer = luaL_checkinteger(L, i);
}

inline void get_float(lua_State* L, float* num) {
	lua_pushnumber(L, *num);
}

inline void set_float(lua_State* L, float* num, int i) {
	*num = luaL_checknumber(L, i);
}

inline void get_vector(lua_State* L, vec2* vector) {
	vec2* vec = lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*vec)[0] = (*vector)[0];
	(*vec)[1] = (*vector)[1];
}

inline void set_vector(lua_State* L, vec2* vector, int i) {
	vec2* vec = luaL_checkudata(L, i, Vector_mt);
	(*vector)[0] = (*vec)[0];
	(*vector)[1] = (*vec)[1];
}

inline void get_texture(lua_State* L, rid_t* texture) {
	*(rid_t*)lua_newuserdata(L, sizeof(rid_t)) = *texture;
	luaL_setmetatable(L, res_mt[RES_TEXTURE]);
}

inline void set_texture(lua_State* L, rid_t* texture, int i) {
	*texture = *(rid_t*)luaL_checkudata(L, i, res_mt[RES_TEXTURE]);
}

//Generate getters and setters for all components 
#define Y(val, func) \
		if (len == sizeof(#val)-1 && !memcmp(key, #val, sizeof(#val)-1)) \
			get_##func(L, &comp->val); \
		else
#define X(name, type, size, attr) \
int name##_index(lua_State *L) { \
	type* comp = *(void**)lua_touserdata(L, 1); \
	int t = lua_type(L, 2); \
	if (t == LUA_TSTRING) { \
		size_t len; \
		const char *key = lua_tolstring(L, 2, &len); \
		attr \
		lua_pushnil(L); \
	} \
	else lua_pushnil(L); \
	return 1; \
}
ENTITY_DATA
#undef X
#undef Y
#define Y(val, func) \
		if (len == sizeof(#val)-1 && !memcmp(key, #val, sizeof(#val)-1)) \
			set_##func(L, &comp->val, 3); \
		else
#define X(name, type, size, attr) \
int name##_newindex(lua_State *L) { \
	type* comp = *(void**)lua_touserdata(L, 1); \
	int t = lua_type(L, 2); \
	if (t == LUA_TSTRING) { \
		size_t len; \
		const char *key = lua_tolstring(L, 2, &len); \
		attr \
		return 0; \
	} \
	return 0; \
}
ENTITY_DATA
#undef X
#undef Y
