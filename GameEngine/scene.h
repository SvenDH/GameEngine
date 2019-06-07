#pragma once
#include "sprite.h"
#include "graphics.h"
#include "data.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>

#define Scene_mt "Scene"
#define MAX_SCENES 64

/* SCENE MANAGMENT */
typedef struct SceneStack {
	int* array_;
	int top_;
	int max_;
} SceneStack;

int openlib_Scene(lua_State* L);