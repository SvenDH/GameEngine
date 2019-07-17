#include "math.h"

//TODO: check for number second arg
//TODO: reduce duplication

//Vector(x, y)
static int vector_new(lua_State* L) {
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	vec2* vec = lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*vec)[0] = x;
	(*vec)[1] = y;
	return 1;
}

//-Vector
static int vector_unm(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*v2)[0] = -(*v1)[0];
	(*v2)[1] = -(*v1)[1];
	return 1;
}

//Vector + Vector
static int vector_add(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)luaL_checkudata(L, 2, Vector_mt);
	vec2* v3 = (vec2*)lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*v3)[0] = (*v1)[0] + (*v2)[0];
	(*v3)[1] = (*v1)[1] + (*v2)[1];
	return 1;
}

//Vector - Vector
static int vector_sub(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)luaL_checkudata(L, 2, Vector_mt);
	vec2* v3 = (vec2*)lua_newuserdata(L, sizeof(vec2));
	luaL_setmetatable(L, Vector_mt);
	(*v3)[0] = (*v1)[0] - (*v2)[0];
	(*v3)[1] = (*v1)[1] - (*v2)[1];
	return 1;
}

//Vector * Vector
static int vector_mul(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)luaL_checkudata(L, 2, Vector_mt);
	float mul = (*v1)[0] * (*v2)[0] + (*v1)[1] * (*v2)[1];
	lua_pushnumber(L, mul);
	return 1;
}

//Vector / Vector

//Vector == Vector
static int vector_eq(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)luaL_checkudata(L, 2, Vector_mt);
	int b = (*v1)[0] == (*v2)[0] && (*v1)[1] == (*v2)[1];
	lua_pushboolean(L, b);
	return 1;
}

//Vector:len()
static int vector_len(lua_State* L) {
	vec2* v1 = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	vec2* v2 = (vec2*)luaL_checkudata(L, 2, Vector_mt);
	float len = sqrt((*v1)[0] * (*v2)[0] + (*v1)[1] * (*v2)[1]);
	lua_pushnumbern(L, len);
	return 1;
}

//Vector.x / Vector.y / Vector[i]
static int vector_index(lua_State *L) {
	vec2* vec = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	int i = INT32_MAX;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "x"))
			i = 0;
		else if (!strcmp(key, "y"))
			i = 1;
		else luaL_error(L, "error invalid key: %s\n", key);
	}
	else if (t == LUA_TNUMBER) {
		i = lua_tointeger(L, 2);
	}
	if (i < 2) lua_pushnumber(L, (*vec)[i]);
	else luaL_error(L, "error out of bounds: %i\n", i);
	return 1;
}

static int vector_newindex(lua_State *L) {
	vec2* vec = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	int i = INT32_MAX;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "x"))
			i = 0;
		else if (!strcmp(key, "y"))
			i = 1;
		else luaL_error(L, "error invalid key: %s\n", key);
	}
	else if (t == LUA_TNUMBER) {
		i = lua_tointeger(L, 2);
	}
	if (i < 2) (*vec)[i] = luaL_checknumber(L, 3);
	else luaL_error(L, "error out of bounds: %i\n", i);
	return 0;
}

static int vector_tostring(lua_State *L) {
	vec2* vec = (vec2*)luaL_checkudata(L, 1, Vector_mt);
	lua_pushfstring(L, "[ %f, %f ]", (*vec)[0], (*vec)[1]);
	return 1;
}

int openlib_Math(lua_State* L) {
	static luaL_Reg vector_func[] = {
		{"__index", vector_index},
		{"__newindex", vector_newindex},
		{"__add", vector_add},
		{"__sub", vector_sub},
		{"__unm", vector_unm},
		{"__mul", vector_mul},
		{"__eq", vector_eq},
		{"__tostring", vector_tostring},
		{NULL, NULL}
	};

	create_lua_class(L, Vector_mt, vector_new, vector_func);
}