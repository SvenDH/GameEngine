#include "scene.h"

SceneStack* scene_stack;

//Scene.push(scene)
static int scene_push(lua_State *L) {
	if (scene_stack->top_ != MAX_SCENES - 1)
		push(scene_stack, luaL_ref(L, LUA_REGISTRYINDEX), int);
	return 0;
}
//Scene.pop()
static int scene_pop(lua_State *L) {
	if (scene_stack->top_ > -1) {
		int r;
		pop(scene_stack, r, int);
		lua_rawgeti(L, LUA_REGISTRYINDEX, r);
		luaL_unref(L, LUA_REGISTRYINDEX, r);
	}
	else lua_pushnil(L);
	return 1;
}
//Scene.peek()
static int scene_peek(lua_State *L) {
	if (scene_stack->top_ > -1) {
		int r = scene_stack->array_[scene_stack->top_];
		lua_rawgeti(L, LUA_REGISTRYINDEX, r);
	}
	else
		lua_pushnil(L);
	return 1;
}
//Scene.clear()
static int scene_clear(lua_State* L) {
	int c = 0;
	for (int i = scene_stack->top_; i > -1; i--) c += scene_pop(L);
	return c;
}
//Scene.update(dt)
static int scene_update(lua_State* L) {
	double dt = luaL_checknumber(L, -1);
	if (scene_stack->top_ == -1)
		async_stop();
	else
		for (int i = scene_stack->top_; i > -1; i--) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, scene_stack->array_[i]);
			lua_getfield(L, -1, "update");
			if (lua_isfunction(L, -1)) {
				lua_pushvalue(L, -2);
				lua_pushnumber(L, dt);
				lua_call(L, 2, 0);
			}
			i = min(scene_stack->top_, i);
		}
}
//Scene.render()
static int scene_render(lua_State* L) {
	for (int i = 0; i < scene_stack->top_+1; i++) {
		lua_settop(L, 0);
		lua_rawgeti(L, LUA_REGISTRYINDEX, scene_stack->array_[i]);
		lua_getfield(L, -1, "draw");
		if (lua_isfunction(L, -1)) {
			lua_pushvalue(L, -2);
			lua_call(L, 1, 0);
		}
		i = min(scene_stack->top_, i);
	}
	return 0;
}

int openlib_Scene(lua_State* L) {
	scene_stack = malloc(sizeof(SceneStack));
	scene_stack->array_ = malloc(MAX_SCENES * sizeof(int));
	scene_stack->top_ = -1;
	scene_stack->max_ = MAX_SCENES;
	lua_settop(L, 0);
	static luaL_Reg lib[] = {
		{"push", scene_push},
		{"pop", scene_pop},
		{"peek", scene_peek},
		{"clear", scene_clear},
		{"update", scene_update},
		{"render", scene_render},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	lua_setglobal(L, Scene_mt);
	return 1;
}
