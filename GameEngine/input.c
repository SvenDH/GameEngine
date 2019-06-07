#pragma once
#include "input.h"
#include "data.h"

static RingBuffer* input_buffer;
static RingBuffer* text_buffer;

int text_enqueue(char c) {
	enqueue(text_buffer, c, char);
}

char text_peek() {
	char out;
	peek(text_buffer, out, char);
	return out;
}

char text_dequeue() {
	char out;
	dequeue(text_buffer, out, char);
	return out;
}

int input_enqueue(int input, double time) {
	InputAction ai;
	ai.button = input;
	ai.time = time;
	enqueue(input_buffer, ai, InputAction);
}

InputAction input_dequeue() {
	InputAction ai = { 0 };
	dequeue(input_buffer, ai, InputAction);
	return ai;
}

InputAction input_peek() {
	InputAction ai = { 0 };
	peek(input_buffer, ai, InputAction);
	return ai;
}

static int input_get(lua_State* L) {
	InputAction input;
	if (lua_gettop(L) > 0) {
		input = input_peek();
		if (input.button != luaL_checkinteger(L, -1))
			input.button = 0;
	}
	else {
		input = input_dequeue();
	}
	if (input.button) {
		lua_pushnumber(L, input.button);
		lua_pushnumber(L, input.time);
		return 2;
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
		return 2;
	}
}
#define E LUA_ENUM_HELPER
int openlib_Input(lua_State* L) {
	input_buffer = ringbuffer_init(INPUT_BUFFER_SIZE, malloc(INPUT_BUFFER_SIZE * sizeof(InputAction)));
	text_buffer = ringbuffer_init(INPUT_BUFFER_SIZE, malloc(INPUT_BUFFER_SIZE * sizeof(char)));

	lua_settop(L, 0);
	lua_newtable(L);
	BUTTONS
	lua_pushliteral(L, "RELEASE"); lua_pushnumber(L, RELEASE_BUTTON); lua_settable(L, -3);
	lua_pushliteral(L, "PRESS"); lua_pushnumber(L, PRESS_BUTTON); lua_settable(L, -3);
	lua_setglobal(L, "INPUT");
	static const struct luaL_Reg func[] = {
		{"get", input_get},
		{NULL, NULL}
	};
	luaL_newlib(L, func);
	lua_setglobal(L, Input_mt);
	return 1;
}
#undef E