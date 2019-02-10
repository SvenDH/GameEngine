#include "scene.h"
#include "sprite.h"
#include "renderer.h"
#include "async.h"
#include "utils.h"

SceneStack scene_stack;

//Scene.push(scene)
static int lua_Scene_push(lua_State *L) {
	if (scene_stack.top_ != MAX_SCENES - 1)
		scene_push(luaL_ref(L, LUA_REGISTRYINDEX));
	return 1;
}
//Scene.pop()
static int lua_Scene_pop(lua_State *L) {
	if (scene_stack.top_ > -1) {
		int r;
		scene_pop(r);
		lua_rawgeti(L, LUA_REGISTRYINDEX, r);
		luaL_unref(L, LUA_REGISTRYINDEX, r);
	}
	else
		lua_pushnil(L);
	return 1;
}
//Scene.peek()
static int lua_Scene_peek(lua_State *L) {
	if (scene_stack.top_ > -1) {
		int r = scene_stack.array_[scene_stack.top_];
		lua_rawgeti(L, LUA_REGISTRYINDEX, r);
	}
	else
		lua_pushnil(L);
	return 1;
}
//Scene.clear()
static int lua_Scene_clear(lua_State* L) {
	int c = 0;
	for (int i = scene_stack.top_; i > -1; i--) c += lua_Scene_pop(L);
	return c;
}
//Scene
int openlib_Scene(lua_State* L) {
	scene_stack.top_ = -1;
	lua_settop(L, 0);
	static luaL_Reg lib[] = {
		{"push", lua_Scene_push},
		{"pop", lua_Scene_pop},
		{"peek", lua_Scene_peek},
		{"clear", lua_Scene_clear},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	lua_setglobal(L, Scene_mt);
	return 1;
}


void scenes_update(lua_State* L, double dt) {
	if (scene_stack.top_ == -1)
		async_stop();
	else
		for (int i = scene_stack.top_; i > -1; i--) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, scene_stack.array_[i]);
			lua_getfield(L, -1, "update");
			if (lua_isfunction(L, -1)) {
				lua_pushvalue(L, -2);
				lua_pushnumber(L, dt);
				lua_call(L, 2, 0);
			}
		}
}

void scenes_render(lua_State* L, int w, int h) {
	renderer_start();
	for (int i = scene_stack.top_; i > -1; i--) {
		renderer_begin();

		lua_settop(L, 0);
		lua_rawgeti(L, LUA_REGISTRYINDEX, scene_stack.array_[i]);
		lua_getfield(L, -1, "sprites");
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			if (lua_isstring(L, -2)) {
				Sprite* spr = (Sprite*)luaL_testudata(L, -1, Sprite_mt);
				if (spr) sprite_draw(spr);
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 3);

		renderer_end();
	}
	renderer_finish();
}