#pragma once
#include "texture.h"
#include "shader.h"
#include "sprite.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int openlib_Render(lua_State* L);
void renderer_start();
void renderer_begin();
void renderer_end();
void renderer_finish();