#pragma once
#include "texture.h"

#define Sprite_mt "Sprite"

#define VERTEX_BUFFER_SIZE 16384

typedef struct vertex {
	float x, y, s, t, i;
	unsigned char r, g, b, a;
} vertex;

inline void quad_new(vertex* vert, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3, float a0, float a1, float a2, float a3);

void sprite_draw_tiles(Texture* tex, const char* tiles, float x, float y, unsigned int columns, unsigned int rows);
void sprite_draw_text(Texture* tex, const char* text, float x, float y, int center, float a);
void sprite_draw_quad(Texture* tex, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c, float a);
void sprite_draw(Texture* texture, vertex* vertices, size_t size);
void sprite_draw_vertices(vertex* vertices, size_t size);
void sprite_flush();

int openlib_Sprite(lua_State* L);