#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//TODO: use memory user for STBI_MALLOC
#include "resource.h"


void image_load(image_t* image, const char* data, size_t len) {
	int w, h, c;
	image->data = stbi_load_from_memory(data, len, &w, &h, &c, 4);
	image->width = w;
	image->height = h;
	image->channels = c;
}

void image_unload(image_t* image) {
	free(image->data);
}

//Image(path)
int w_image_new(lua_State* L) {
	lua_pushinteger(L, RES_IMAGE);
	return w_resource_new(L);
}

//Image.__load(data)
int w_image_load(lua_State* L) {
	image_t* image = (image_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	size_t len;
	char* data = luaL_checklstring(L, 2, &len);
	image_load(image, data, len);
	return 0;
}

//Image.unload()
int w_image_unload(lua_State* L) {
	image_t* image = (image_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	image_unload(image);
	return 0;
}