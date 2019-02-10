#include "script.h"

lua_State* L;

void script_load(Script* script, const char* name, const char* code) {
	if (luaL_dostring(L, code)) {
		printf("Error loading script %s: %s\n", name, lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	else {
		lua_setglobal(L, name);
		script->result = name;
	}
}


int openlib_Script(lua_State* default_lua) {
	L = default_lua;
	luaL_newmetatable(L, Script_mt);
	lua_pop(L, 1);
	return 1;
}