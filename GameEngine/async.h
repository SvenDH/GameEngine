#pragma once
#include "data.h"
#include "event.h"
#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define App_mt "App"
#define Timer_mt "Timer"

typedef struct Timer {
	uv_timer_t handle;
	lua_State* callstate;
	int callback;
	double lasttime;
} Timer;

int openlib_Async(lua_State* L);