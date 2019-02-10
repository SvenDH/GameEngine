#pragma once
#include "sprite.h"
#include "texture.h"
#include "shader.h"

#include <glad.h>
#include <lauxlib.h>
#include <cglm/cglm.h>

typedef struct Camera {
	vec3 pos;
	vec3 size;
	mat4 view;
	mat4 projection;
} Camera;

typedef struct Canvas {
	Sprite spr;
	unsigned int FBO;
	Shader* shader;
	Camera* camera;
} Canvas;

void canvas_new(Canvas* canvas, int width, int height, int zoom, Shader* shader, Camera* camera);
void canvas_bind(Canvas* canvas);
void camera_new(Camera* camera, float x, float y, float width, float height);