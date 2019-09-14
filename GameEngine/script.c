#include "resource.h"

void script_load(script_t* script, lua_State* L, const char* data, size_t len) {
	if (luaL_loadbuffer(L, data, len, NULL)) {
		printf("Error loading script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	script->L = L;
	script->ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

void script_unload(script_t* script) {
	lua_rawgeti(script->L, LUA_REGISTRYINDEX, script->ref);
	luaL_unref(script->L, LUA_REGISTRYINDEX, script->ref);
}

int script_run(script_t* script) { //TODO: multiple arguments as input
	lua_State* L = script->L;
	lua_rawgeti(L, LUA_REGISTRYINDEX, script->ref);
	if (lua_pcall(L, 0, 1, 0)) {
		printf("Error running script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	return 1;
}

//Script(path)
int w_script_new(lua_State* L) {
	lua_pushinteger(L, RES_SCRIPT);
	return w_resource_new(L);
}

//Script:__load("data")
int w_script_load(lua_State* L) {
	script_t* script = (script_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	size_t len;
	char* data = luaL_checklstring(L, 2, &len);
	script_load(script, L, data, len);
	return 0;
}

//Script:__unload()
int w_script_unload(lua_State* L) {
	script_t* script = (script_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	script_unload(script);
	return 0;
}

//Script:run()
int w_script_run(lua_State* L) { //TODO: multiple arguments as input
	script_t* script = (script_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	if (!script->loaded) LOAD_RESOURCE(L, 1);
	return script_run(script);
}