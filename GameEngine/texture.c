#include "texture.h"
#include "utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture* current_texture = NULL;

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

static int texture_load(lua_State *L) {
	int spr_w, spr_h, w, h, c, data_len;
	unsigned char* image;
	Texture* tex;

	const char* data = luaL_checklstring(L, 1, &data_len);
	spr_w = luaL_checkinteger(L, 2);
	spr_h = luaL_checkinteger(L, 3);
	if (spr_w < 0 || spr_h < 0) return luaL_error(L, "Error dimensions incorrect");

	image = stbi_load_from_memory(data, data_len, &w, &h, &c, 4);
	if (image) {
		tex = (Texture*)lua_newuserdata(L, sizeof(Texture));
		luaL_setmetatable(L, Texture_mt);
		texture_generate(tex, spr_w, spr_h, (w / spr_w) * (h / spr_h), 4, NULL);
		texture_sheet(tex, w, h, image);
		free(image);
		return 1;
	}
	else return luaL_error(L, "Error loading sprite image data");
}

static int lua_Texture_index(lua_State* L) {
	Texture *s = luaL_checkudata(L, 1, Texture_mt);
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

static int lua_Texture_bind(lua_State* L) {
	Texture *s = luaL_checkudata(L, 1, Texture_mt);
	texture_bind(s);
	return 0;
}

static luaL_Reg func[] = {
		{"__index", lua_Texture_index},
		{"bind", lua_Texture_bind},
		{NULL, NULL}
};

static luaL_Reg lib[] = {
		{"load", texture_load},
		{NULL, NULL}
};

int openlib_Texture(lua_State* L) {
	luaL_newmetatable(L, Texture_mt);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, func, 0);
	lua_pop(L, 1);

	luaL_newlib(L, lib);
	lua_setglobal(L, Texture_mt);
	return 1;
}
