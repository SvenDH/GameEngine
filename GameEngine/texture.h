#pragma once
#include <glad.h>
#include <lauxlib.h>

#define Texture_mt "Texture"

int openlib_Texture(lua_State* L);

typedef struct Texture {
	GLuint tex_ID;
	GLenum format;
	int width, height, depth;
} Texture;

void texture_generate(Texture* tex, int tex_w, int tex_h, int tex_d, int channels, unsigned char* data);
void texture_sheet(Texture* tex, int sheet_w, int sheet_h, unsigned char* data);
void texture_subimage(Texture* tex, int depth, unsigned char* data);
void texture_delete(Texture* tex);
void texture_bind(Texture* tex);
Texture* texture_getcurrent();

Texture *checktexture(lua_State *L, int i);
static int Texture_mt_index(lua_State* L);