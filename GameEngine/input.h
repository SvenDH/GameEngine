#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

#define Input_mt "Input"
#define INPUT_BUFFER_SIZE 256

#define BUTTONS \
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
	PRESS_BUTTON = 0x00,
	RELEASE_BUTTON = 0x80
};
#undef E

typedef struct {
	int button;
	double time;
} InputAction;

typedef struct {
	InputAction buffer_[INPUT_BUFFER_SIZE];
	int head_;
	int tail_;
} InputBuffer;

int input_enqueue(int input, double time);
InputAction* input_dequeue();

int openlib_Input(lua_State* L);