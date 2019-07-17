#pragma once
#include "utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <lua.h>

#define Window_mt "Window"
#define Input_mt "Input"

#define BUTTONS \
	E(NO_BUTTON, NONE) \
	E(UP_BUTTON, UP) \
	E(DOWN_BUTTON, DOWN) \
	E(LEFT_BUTTON, LEFT) \
	E(RIGHT_BUTTON, RIGHT) \
	E(A_BUTTON, A) \
	E(B_BUTTON, B) \
	E(X_BUTTON, X) \
	E(Y_BUTTON, Y) \
	E(MENU_BUTTON, MENU) \
	E(SELECT_BUTTON, SELECT)

#define E C_ENUM_HELPER
typedef enum Button {
	BUTTONS
	MAX_BUTTONS,
	PRESS_BUTTON = 0x80,
	RELEASE_BUTTON = 0x40,
	REPEAT_BUTTON = 0x20
};
#undef E

void window_swap();
void window_poll();
void window_close();

int openlib_Window(lua_State* L);