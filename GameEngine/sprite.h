#pragma once
#include "texture.h"

#define Sprite_mt "Sprite"

#define VERTEX_BUFFER_SIZE 1024

#define sprite_ctor(_spr, _tex, _size)  _spr->texture = _tex; \
										_spr->vertices = malloc(_size * 4 * sizeof(vertex)); \
										_spr->size = _size

typedef struct vertex {
	float x, y, s, t, i;
	unsigned char r, g, b, a;
} vertex;

typedef struct Sprite {
	Texture* texture;
	vertex* vertices;
	size_t size;
 } Sprite;

inline void quad_new(vertex* vert, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3, float a0, float a1, float a2, float a3);
void sprite_new(Sprite* spr, Texture* tex, unsigned int i, float x, float y, float w, float h, unsigned int c, float a);
void sprite_new_ext(Sprite* spr, Texture* tex, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c, float a);
void sprite_new_text(Sprite* spr, Texture* tex, const char* text, float x, float y, int center, float a);

void sprite_draw(Sprite* spr);
void sprite_flush();

Sprite *checksprite(lua_State *L, int i);

int openlib_Sprite(lua_State* L);