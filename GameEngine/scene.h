#pragma once
#include "sprite.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>

#define Scene_mt "Scene"
#define MAX_SCENES 64

#define scene_push(_r_) scene_stack.array_[++scene_stack.top_] = _r_
#define scene_pop(_r_) _r_ = scene_stack.array_[scene_stack.top_--]

/* SCENE MANAGMENT */
typedef struct SceneStack {
	int* array_[MAX_SCENES];
	int top_;
} SceneStack;

int openlib_Scene(lua_State* L);
static int lua_Scene_new(lua_State* L);
static int lua_Scene_push(lua_State *L);
static int lua_Scene_pop(lua_State *L);
static int lua_Scene_peek(lua_State *L);
static int lua_Scene_clear(lua_State* L);

void scenes_update(lua_State* L, double dt);
void scenes_render(lua_State* L, int w, int h);