#pragma once
#include "texture.h"
#include "shader.h"
#include "sprite.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

#define Graphics_mt "Graphics"
#define Canvas_mt "Canvas"
#define Camera_mt "Camera"

typedef struct Canvas {
	Texture* tex;
	unsigned int FBO, RBO;
} Canvas;

typedef struct Camera {
	vec3 pos;
	vec3 size;
	mat4 view;
	mat4 projection;
	Shader* shader;
	Canvas* canvas;
} Camera;

void camera_new(Camera* camera, float x, float y, float width, float height);
void camera_set(Camera* camera);

void canvas_bind(Canvas* canvas);

int openlib_Canvas(lua_State* L);

int openlib_Graphics(lua_State* L);
