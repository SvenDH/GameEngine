#include "component.h"

void get_integer(lua_State* L, int* integer) {
	lua_pushinteger(L, *integer);
}

void set_integer(lua_State* L, int* integer, int i) {
	*integer = luaL_checkinteger(L, i);
}

void get_vector(lua_State* L, vec2* vector) {
	vec2* vec = lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*vec)[0] = (*vector)[0];
	(*vec)[1] = (*vector)[1];
}

void set_vector(lua_State* L, vec2* vector, int i) {
	vec2* vec = luaL_checkudata(L, i, Vector_mt);
	(*vector)[0] = (*vec)[0];
	(*vector)[1] = (*vec)[1];
}

void get_texture(lua_State* L, Texture** tex) {
	Texture** ref = lua_newuserdata(L, sizeof(Texture*));
	luaL_setmetatable(L, Texture_mt);
	*ref = *tex;
}

void set_texture(lua_State* L, Texture** tex, int i) {
	Texture* ref = *(Texture**)luaL_checkudata(L, i, Texture_mt);
	*tex = ref;
}

//Generate getters and setters for all components 
#define Z(val, func) \
		if (strcmp(key, #val) == 0) \
			func(L, &comp->val, 3); \
		else
#define Y(val, func) \
		if (strcmp(key, #val) == 0) \
			func(L, &comp->val); \
		else
#define X(name, type, getter, setter) \
int name##_index(lua_State *L) { \
	type* comp = *(type**)lua_touserdata(L, 1); \
	int t = lua_type(L, 2); \
	if (t == LUA_TSTRING) { \
		const char *key = lua_tostring(L, 2); \
		getter \
		lua_pushnil(L); \
	} \
	else lua_pushnil(L); \
	return 1; \
} \
\
int name##_newindex(lua_State *L) { \
	type* comp = *(type**)lua_touserdata(L, 1); \
	int t = lua_type(L, 2); \
	if (t == LUA_TSTRING) { \
		const char *key = lua_tostring(L, 2); \
		setter \
		return 0; \
	} \
	return 0; \
}
ENTITY_DATA
#undef X
#undef Y
#undef Z
