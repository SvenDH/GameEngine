#include "texture.h"
#include "utils.h"

Texture* current_texture = NULL;

int openlib_Texture(lua_State* L) {
	luaL_newmetatable(L, Texture_mt);
	static const struct luaL_Reg lib[] = {
		{"__index", Texture_mt_index},
		{NULL, NULL}
	};
	luaL_setfuncs(L, lib, 0);
	lua_pop(L, 1);
	return 1;
}

void texture_generate(Texture* tex, int tex_w, int tex_h, int tex_d, int channels, unsigned char* data) {
	if (tex_w > GL_MAX_TEXTURE_SIZE || tex_h > GL_MAX_TEXTURE_SIZE) {
		printf("Sprite: Image too large: %ix%i, max size: %i", tex_w, tex_h, GL_MAX_TEXTURE_SIZE);
		return;
	}
	tex->width = tex_w;
	tex->height = tex_h;
	tex->depth = tex_d;
	
	switch (channels) {
	case 1: tex->format = GL_RED; break;
	case 2: tex->format = GL_RG; break;
	case 3: tex->format = GL_RGB; break;
	case 4: default: tex->format = GL_RGBA;  break;
	}

	if (tex->depth > GL_MAX_ARRAY_TEXTURE_LAYERS) {
		printf("Sprite: Too many sub images: %i, max images: %i", tex->depth, GL_MAX_ARRAY_TEXTURE_LAYERS);
		return;
	}
	glGenTextures(1, &tex->tex_ID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex->tex_ID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (data) glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, tex->width, tex->height, tex->depth, 0, tex->format, GL_UNSIGNED_BYTE, data);
	else glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, tex->width, tex->height, tex->depth, 0, tex->format, GL_UNSIGNED_BYTE, 0);
}

void texture_sheet(Texture* tex, int sheet_w, int sheet_h, unsigned char* data) {
	int columns = sheet_w / tex->width;
	int rows = sheet_h / tex->height;

	glBindTexture(GL_TEXTURE_2D_ARRAY, tex->tex_ID);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, sheet_w);
	for (int x = 0; x < columns; x++) {
		for (int y = 0; y < rows; y++) {
			int xoff = x * tex->width;
			int yoff = y * tex->height;
			int depth = y * columns + x;
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, xoff);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, yoff);
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, depth, tex->width, tex->height, 1, tex->format, GL_UNSIGNED_BYTE, data);
		}
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

void texture_subimage(Texture* sprite, int index, unsigned char* data) {
	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->tex_ID);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, sprite->width, sprite->height, 1, sprite->format, GL_UNSIGNED_BYTE, data);
}

void texture_delete(Texture* sprite) {
	glDeleteTextures(1, &sprite->tex_ID);
}

void texture_bind(Texture* tex) {
	if (tex != current_texture) {
		if (tex) glBindTexture(GL_TEXTURE_2D_ARRAY, tex->tex_ID);
		else glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		current_texture = tex;
	}
}

Texture* texture_getcurrent() {
	return current_texture;
}

Texture *checktexture(lua_State *L, int i) {
	void *ud = luaL_checkudata(L, i, Texture_mt);
	luaL_argcheck(L, ud != NULL, i, "Texture expected");
	return (Texture*)ud;
}

static int Texture_mt_index(lua_State* L) {
	Texture *s = checktexture(L, 1);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);		
		if (!strcmp(key, "width") || !strcmp(key, "w")) lua_pushinteger(L, s->width);
		else if (!strcmp(key, "height") || !strcmp(key, "h")) lua_pushinteger(L, s->height);
		else if (!strcmp(key, "depth") || !strcmp(key, "d")) lua_pushinteger(L, s->depth);
	}
	else lua_pushnil(L);
	return 1;
}