#include "async.h"
#include "utils.h"

static void timer_cb(uv_timer_t* handle);

static int timer_new(lua_State* L) {
	double timeout = luaL_checknumber(L, 1);
	double repeat = luaL_checknumber(L, 2);
	int cb = luaL_ref(L, LUA_REGISTRYINDEX);

	Timer *timer = (Timer*)lua_newuserdata(L, sizeof(Timer));
	luaL_setmetatable(L, Timer_mt);
	timer->callstate = L;
	timer->callback = cb;
	timer->lasttime = get_time();
	uv_timer_init(uv_default_loop(), (uv_timer_t*)timer);
	uv_timer_start((uv_timer_t*)timer, timer_cb, (int)(timeout * 1000), (int)(repeat * 1000));
	return 1;
}

static int timer_delete(lua_State* L) {
	Timer* timer = luaL_checkudata(L, 1, Timer_mt);
	luaL_unref(L, LUA_REGISTRYINDEX, timer->callback);
	uv_timer_stop(timer);
	return 0;
}

static void timer_cb(uv_timer_t* handle) {
	Timer* timer = (Timer*)handle;
	double current = get_time();
	lua_rawgeti(timer->callstate, LUA_REGISTRYINDEX, timer->callback);
	if (lua_isfunction(timer->callstate, -1)) {
		lua_pushnumber(timer->callstate, current - timer->lasttime);
		lua_call(timer->callstate, 1, 0);
	}
	timer->lasttime = current;
}

static const struct luaL_Reg lib[] = {
		{"__gc", timer_delete},
		{NULL, NULL}
};

static luaL_Reg func[] = {
		{"new", timer_new},
		{NULL, NULL}
};

int openlib_Timer(lua_State* L) {
	luaL_newmetatable(L, Timer_mt);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lib, 0);
	lua_pop(L, 1);

	luaL_newlib(L, func);
	lua_setglobal(L, Timer_mt);
}