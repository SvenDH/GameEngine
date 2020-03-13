#pragma once
#include "types.h"
#include "data.h"
#include "event.h"
#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define App_mt "app"
#define Timer_mt "timer"
#define Window_mt "window"
#define Input_mt "input"

#define TICKS_PER_SECOND 25
#define SKIP_TICKS (1000 / TICKS_PER_SECOND)
#define MAX_FRAMESKIP 5

typedef struct {
	GLFWwindow* glfw;
	int width, height;
	char title[256];
} window_t;

typedef struct {
	const char* name;
	int next_game_tick;
	int is_running;
	char keys[1024];
} app_t;

typedef void(*Timer_cb) (void *, double);

typedef struct timer_t {
	uv_timer_t handle;
	double lasttime;
	Timer_cb cb;
	void* data;
	int use_lua;
} timer_t;

timer_t* timer_new(double timeout, double repeat, Timer_cb cb, void* data);
void timer_delete(timer_t* timer);

void app_init(app_t* manager, const char* name);
int app_run(app_t* app);
int app_stop(app_t* app, event_t evt);

inline GLOBAL_SINGLETON(app_t, app, "App");

int openlib_App(lua_State* L);

void window_init(window_t* window, const char* name);
void window_resize(window_t* window, int width, int height);
void window_title(window_t* window, const char* title);
void window_swap(window_t* window);
void window_poll(window_t* window);
void window_close(window_t* window);

inline GLOBAL_SINGLETON(window_t, window, "Window");

int openlib_Window(lua_State* L);