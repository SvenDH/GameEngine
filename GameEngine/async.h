#pragma once
#include "data.h"
#include "event.h"
#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define App_mt "app"
#define Timer_mt "timer"

#define TICKS_PER_SECOND 25
#define SKIP_TICKS (1000 / TICKS_PER_SECOND)
#define MAX_FRAMESKIP 5

typedef struct {
	char* name;
	int next_game_tick;
	int is_running;
} app_t;

typedef void(*Timer_cb) (void *, double);

typedef struct timer_t {
	uv_timer_t handle;
	double lasttime;
	Timer_cb cb;
	void* data;
	int use_lua;
} timer_t;

void app_init(app_t* manager, const char* name);
void app_run(app_t* app);
void app_stop(app_t* app);

inline GLOBAL_SINGLETON(app_t, app, "App");

timer_t* timer_new(double timeout, double repeat, Timer_cb cb, void* data);
void timer_delete(timer_t* timer);

int openlib_App(lua_State* L);