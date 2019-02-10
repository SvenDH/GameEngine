#pragma once
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Script_mt "Script"

typedef struct Script {
	char* result;
} Script;

void script_load(Script* script, const char* name, const char* code);

int openlib_Script(lua_State* L);