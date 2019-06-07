#pragma once
#include "data.h"
#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Timer_mt "Timer"

typedef struct Timer {
	uv_timer_t handle;
	double lasttime;
	lua_State* callstate;
	int callback;
} Timer;

int openlib_Timer(lua_State* L);

inline void async_run() {
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

inline void async_stop() {
	uv_stop(uv_default_loop());
}