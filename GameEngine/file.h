#pragma once
#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define File_mt "File"
#define Dir_mt "Dir"

typedef struct File {
	uv_file fd;
	uv_fs_t* req;
	int callback;
	lua_State* callstate;
	uv_buf_t buf;
} File;

typedef struct Dir {
	uv_fs_t* req;
} Dir;

int openlib_File(lua_State* L);