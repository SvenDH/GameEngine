#include "graphics.h"
#include "math.h"
#include "utils.h"

static GLuint current = 0;

void texture_load(texture_t* texture, image_t* image, int width, int height) {
	int depth = (image->width / width) * (image->height / height);
	texture_generate(texture, width, height, depth, 4, NULL);
	texture_sheet(texture, image->width, image->height, image->data);
}

void texture_unload(texture_t* texture) {
	glDeleteTextures(1, &texture->id);
}

void texture_generate(texture_t* texture, int width, int height, int depth, int channels, const char* data) {
	if (width > GL_MAX_TEXTURE_SIZE || height > GL_MAX_TEXTURE_SIZE) {
		printf("error: Image too large: %ix%i, max size: %i", width, height, GL_MAX_TEXTURE_SIZE);
		return;
	}
	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	
	switch (channels) {
	case 1: texture->format = GL_RED; break;
	case 2: texture->format = GL_RG; break;
	case 3: texture->format = GL_RGB; break;
	case 4: default: texture->format = GL_RGBA;  break;
	}

	if (texture->depth > GL_MAX_ARRAY_TEXTURE_LAYERS) {
		printf("error: Too many sub images: %i, max images: %i", texture->depth, GL_MAX_ARRAY_TEXTURE_LAYERS);
		return;
	}
	glGenTextures(1, &texture->id);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->id);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (data) glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, texture->width, texture->height, texture->depth, 0, texture->format, GL_UNSIGNED_BYTE, data);
	else glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, texture->width, texture->height, texture->depth, 0, texture->format, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D_ARRAY, current);
}

void texture_delete(texture_t* texture) {
	glDeleteTextures(1, &texture->id);
}

void texture_sheet(texture_t* texture, int sheet_w, int sheet_h, const char* data) {
	int columns = sheet_w / texture->width;
	int rows = sheet_h / texture->height;

	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->id);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, sheet_w);
	for (int x = 0; x < columns; x++) {
		for (int y = 0; y < rows; y++) {
			int xoff = x * texture->width;
			int yoff = y * texture->height;
			int depth = y * columns + x;
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, xoff);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, yoff);
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, depth, texture->width, texture->height, 1, texture->format, GL_UNSIGNED_BYTE, data);
		}
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
}

void texture_subimage(texture_t* texture, int index, const char* data) {
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->id);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, texture->width, texture->height, 1, texture->format, GL_UNSIGNED_BYTE, data);
}

int texture_bind(texture_t* tex) {
	GLuint id = 0;
	if (tex) id = tex->id;
	int changed = (id != current);
	if (changed) {
		glBindTexture(GL_TEXTURE_2D_ARRAY, id);
		current = id;
	}
	return changed;
}

//Texture{image, width, height}
int w_texture_new(lua_State* L) {
	lua_pushinteger(L, RES_TEXTURE);
	return w_resource_new(L);
}

//Texture:__load{image, width, heigth}
int w_texture_load(lua_State* L) {
	texture_t* texture = (texture_t*)resource_get(*(rid_t*)lua_touserdata(L, 1));
	if (lua_type(L, 2) == LUA_TTABLE) {
		lua_getfield(L, 2, "image"); //TODO: could also be a path to image maybe
		lua_getfield(L, 2, "width");
		lua_getfield(L, 2, "height");
		lua_remove(L, 2);
	}
	image_t* image = (image_t*)resource_get(*(rid_t*)lua_touserdata(L, 2));
	int w = luaL_optinteger(L, 3, image->width);
	int h = luaL_optinteger(L, 4, image->height);
	if (!image->loaded) LOAD_RESOURCE(L, 2);
	texture_load(texture, image, w, h);
	return 0;
}

//Texture:__unload()
int w_texture_unload(lua_State* L) {
	texture_t* texture = (texture_t*)resource_get(*(rid_t*)lua_touserdata(L, 1));
	texture_unload(texture);
	return 0;
}

//Texture.width / Texture.height
int w_texture_index(lua_State* L) {
	texture_t* texture = (texture_t*)resource_get(*(rid_t*)lua_touserdata(L, 1));
	if (lua_type(L, 2) == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
#define KEY "width"
		if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
			lua_pushinteger(L, texture->width);
		}
#define KEY "height"
		else if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
			lua_pushinteger(L, texture->height);
		}
#undef KEY
		else luaL_getmetafield(L, 1, key);
	}
	else lua_pushnil(L);
	return 1;
}

int w_texture_draw(lua_State* L) {
	return w_graphics_texture(L);
}