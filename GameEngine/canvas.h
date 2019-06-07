#pragma once
#include "sprite.h"
#include "texture.h"
#include "shader.h"

#include <lauxlib.h>
#include <glad/glad.h>

#define Canvas_mt "Canvas"

typedef struct Canvas {
	Sprite spr;
	unsigned int FBO, RBO;
} Canvas;

void canvas_new(Canvas* canvas, int width, int height);
void canvas_bind(Canvas* canvas);

int openlib_Canvas(lua_State* L);