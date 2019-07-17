#include "texture.h"
#include "utils.h"
#include "data.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture* current_texture;
Texture* default_texture;

static GLubyte default_texture_data[] = { 255, 255, 255, 255 };

MemoryPool* texture_pool;

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

void texture_subimage(Texture* tex, int index, unsigned char* data) {
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex->tex_ID);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, tex->width, tex->height, 1, tex->format, GL_UNSIGNED_BYTE, data);
}

void texture_bind(Texture* tex) {
	if (!tex) {
		if (!default_texture) {
			default_texture = malloc(sizeof(Texture));
			texture_generate(default_texture, 1, 1, 1, 4, default_texture_data);
		}
		tex = default_texture;
	}
	if (tex != current_texture) {
		glBindTexture(GL_TEXTURE_2D_ARRAY, tex->tex_ID);
		current_texture = tex;
	}
}

Texture* texture_getcurrent() {
	return current_texture;
}

//Texture(data)
static int texture_load(lua_State *L) {
	int spr_w, spr_h, w, h, c, data_len;
	unsigned char* image;

	const char* data = luaL_checklstring(L, 2, &data_len);
	spr_w = luaL_checkinteger(L, 3);
	spr_h = luaL_checkinteger(L, 4);
	if (spr_w < 0 || spr_h < 0) return luaL_error(L, "Error dimensions incorrect");

	image = stbi_load_from_memory(data, data_len, &w, &h, &c, 4);
	if (image) {
		Texture* tex = pool_alloc(texture_pool);
		texture_generate(tex, spr_w, spr_h, (w / spr_w) * (h / spr_h), 4, NULL);
		texture_sheet(tex, w, h, image);
		free(image);

		Texture** ref = (Texture**)lua_newuserdata(L, sizeof(Texture*));
		luaL_setmetatable(L, Texture_mt);
		*ref = tex;
		return 1;
	}
	else return luaL_error(L, "Error loading sprite image data");
}

//Texture["widht"/"height"/"depth"]
static int texture_index(lua_State* L) {
	Texture *s = *(Texture**)luaL_checkudata(L, 1, Texture_mt);
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

//Texture:set()
static int texture_set(lua_State* L) {
	Texture *tex = *(Texture**)luaL_checkudata(L, 1, Texture_mt);
	texture_bind(tex);
	return 0;
}

//Texture:delete()
static int texture_delete(lua_State* L) {
	Texture *tex = *(Texture**)luaL_checkudata(L, 1, Texture_mt);
	glDeleteTextures(1, &tex->tex_ID);
	pool_free(texture_pool, tex);
	return 0;
}

int openlib_Texture(lua_State* L) {
	texture_pool = pool_init(sizeof(Texture), 32);

	static luaL_Reg tex_func[] = {
		{"__index", texture_index},
		{"delete", texture_delete},
		{"set", texture_set},
		{NULL, NULL}
	};
	create_lua_class(L, Texture_mt, texture_load, tex_func);
	return 0;
}
