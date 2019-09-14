#pragma once
#include "types.h"
#include "event.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <lua.h>

#define Window_mt "window"
#define Input_mt "input"

typedef struct {
	GLFWwindow* glfw;
	int width, height;
	char title[256];
} window_t;

#define BUTTONS \
	X(NO_BUTTON, NONE) \
	X(UP_BUTTON, UP) \
	X(DOWN_BUTTON, DOWN) \
	X(LEFT_BUTTON, LEFT) \
	X(RIGHT_BUTTON, RIGHT) \
	X(A_BUTTON, A) \
	X(B_BUTTON, B) \
	X(X_BUTTON, X) \
	X(Y_BUTTON, Y) \
	X(MENU_BUTTON, MENU) \
	X(SELECT_BUTTON, SELECT)

typedef enum Button {
#define X(name) name,
	BUTTONS
#undef X
	MAX_BUTTONS,
	PRESS_BUTTON = 0x80,
	RELEASE_BUTTON = 0x40,
	REPEAT_BUTTON = 0x20
};

void window_init(window_t* window, const char* name);
void window_resize(window_t* window, int width, int height);
void window_title(window_t* window, const char* title);
void window_swap(window_t* window);
void window_poll(window_t* window);
void window_close(window_t* window);

inline GLOBAL_SINGLETON(window_t, window, "Window");

int openlib_Window(lua_State* L);