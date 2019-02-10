#pragma once
#include "input.h"

static InputBuffer input_buffer;

int input_enqueue(int input, double time) {
	if ((input_buffer.tail_ + 1) % INPUT_BUFFER_SIZE != input_buffer.head_) {
		input_buffer.tail_ = (input_buffer.tail_ + 1) % INPUT_BUFFER_SIZE;
		input_buffer.buffer_[input_buffer.tail_].button = input;
		input_buffer.buffer_[input_buffer.tail_].time = time;
		return 0;
	}
	return 1;
}

InputAction* input_dequeue() {
	if (input_buffer.head_ != input_buffer.tail_) {
		InputAction* ia = &input_buffer.buffer_[input_buffer.head_];
		input_buffer.head_ = (input_buffer.head_ + 1) % INPUT_BUFFER_SIZE;
		return ia;
	}
	return NULL;
}

InputAction* input_peek() {
	if (input_buffer.head_ != input_buffer.tail_)
		return &input_buffer.buffer_[input_buffer.head_];
	return NULL;
}

static int lua_Input_get(lua_State* L) {
	InputAction* input;
	if (lua_gettop(L) > 0) {
		input = input_peek();
		if (!input || input->button != luaL_checkinteger(L, -1))
			input = NULL;
	}
	else {
		input = input_dequeue();
	}
	if (input) {
		lua_pushnumber(L, input->button);
		lua_pushnumber(L, input->time);
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
	lua_settop(L, 0);
	lua_newtable(L);
	BUTTONS
	lua_pushliteral(L, "RELEASE"); lua_pushnumber(L, RELEASE_BUTTON); lua_settable(L, -3);
	lua_pushliteral(L, "PRESS"); lua_pushnumber(L, PRESS_BUTTON); lua_settable(L, -3);
	lua_setglobal(L, "INPUT");
	static const struct luaL_Reg func[] = {
		{"get", lua_Input_get},
		{NULL, NULL}
	};
	luaL_newlib(L, func);
	lua_setglobal(L, Input_mt);
	return 1;
}
#undef E