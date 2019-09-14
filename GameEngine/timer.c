#include "async.h"
#include "utils.h"

static void timer_cb(uv_timer_t* handle);

timer_t* timer_new(double timeout, double repeat, Timer_cb cb, void* data) {
	timer_t *timer = malloc(sizeof(timer_t));
	timer->lasttime = get_time();
	timer->cb = cb;
	timer->data = data;
	uv_timer_init(uv_default_loop(), (uv_timer_t*)timer);
	uv_timer_start((uv_timer_t*)timer, timer_cb, (int)timeout, (int)repeat);
	return 1;
}

void timer_delete(timer_t* timer) {
	uv_timer_stop(timer);
	free(timer);
}

//Timer callback(dt)
static void timer_cb(uv_timer_t* handle) {
	timer_t* timer = (timer_t*)handle;
	double current = get_time();
	if (timer->use_lua) {
		lua_State* L = (lua_State*)timer->data;
		lua_rawgeti(L, LUA_REGISTRYINDEX, timer->cb);
		lua_pushnumber(L, current - timer->lasttime);
		lua_call(L, 1, 0);
	}
	else {
		timer->cb(timer->data, current - timer->lasttime);
	}
	timer->lasttime = current;
	if (!handle->repeat)
		timer_delete(timer);
}

//Timer(timeout, repeat, callback)
static int timer_call(lua_State* L) {
	double timeout = luaL_checknumber(L, 2);
	double repeat = luaL_checknumber(L, 3);
	if (lua_isfunction(L, 4)) {
		int cb = luaL_ref(L, LUA_REGISTRYINDEX);
		timer_t* timer = timer_new(timeout, repeat, cb, L);
		PUSH_LUA_POINTER(L, Timer_mt, timer);
		return 1;
	}
	else luaL_error(L, "error callback is no function");
}

//Timer.__gc()
static int timer_gc(lua_State* L) {
	timer_t* timer = *(timer_t**)luaL_checkudata(L, 1, Timer_mt);
	luaL_unref(L, LUA_REGISTRYINDEX, timer->cb);
	timer_delete(timer);
	return 0;
}

int openlib_Timer(lua_State* L) {
	static luaL_Reg timer_lib[] = {
		{"__gc", timer_gc},
		{NULL, NULL}
	};
	create_lua_class(L, Timer_mt, timer_call, timer_lib);
	return 0;
}