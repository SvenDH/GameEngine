#include "resources.h"
#include "data.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

lua_State* L;

void resources_add(Resource* r) {
	lua_settop(L, 0);
	lua_getglobal(L, Resource_mt);
	lua_pushstring(L, r->name);
	lua_pushlightuserdata(L, r);
	switch (r->type) {
	case TEXTURE: luaL_getmetatable(L, Texture_mt); break;
	case SHADER: luaL_getmetatable(L, Shader_mt); break;
	case SCRIPT: luaL_getmetatable(L, Script_mt); break;
	}
	lua_setmetatable(L, -2);
	lua_settable(L, -3);
}

Resource* resources_get(const char* name) {
	lua_getglobal(L, Resource_mt);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	return (Resource*)lua_touserdata(L, -1);
}

static void load_shader(File* file) {
	Resource* r = (Resource*)file->data;
	Buffer* b = (Buffer*)file->buf.base;
	b->data[b->len] = '\0';
	shader_compile((Shader*)r, b->data);
}

static void load_script(File* file) {
	Resource* r = (Resource*)file->data;
	Buffer* b = (Buffer*)file->buf.base;
	b->data[b->len] = '\0';
	script_load((Script*)r, r->params[0], b->data);
}

static void load_texture(File* file) {
	int spr_w, spr_h, w, h, c;
	unsigned char* image;
	Resource* r = (Resource*)file->data;
	Buffer* b = (Buffer*)file->buf.base;

	spr_w = atoi(r->params[1]);
	spr_h = atoi(r->params[2]);
	image = stbi_load_from_memory(b->data, b->len+1, &w, &h, &c, 4);
	if (image) {
		texture_generate(r, spr_w, spr_h, (w / spr_w) * (h / spr_h), 4, NULL);
		texture_sheet(r, w, h, image);
	}
	else fprintf(stderr, "Error loading sprite image at: %s/%s\n", file->path, file->name);
	free(image);
	file_close(file);
}


static void open_shader(File* file) {
	RESOURCE_INIT((Resource*)file->data, file->name, SHADER);
	file_read(file, load_shader, buffer_alloc());
}

static void open_script(File* file) {
	RESOURCE_INIT((Resource*)file->data, file->name, SCRIPT);
	file_read(file, load_script, buffer_alloc());
}

static void open_texture(File* file) {
	RESOURCE_INIT((Resource*)file->data, file->name, TEXTURE);
	file_read(file, load_texture, buffer_alloc());
}

static int lua_Resources_init(lua_State* L) {
	ResourceLoader resource_loaders[] = {
		{NULL, "../resources/shaders", open_shader},
		{NULL, "../resources/sprites", open_texture},
		{NULL, "../resources/scripts", open_script},
		{NULL, NULL, NULL}
	};

	for (int i = 0; i < MAX_RES_TYPES - 1; i++) {
		resource_loaders[i].dir = malloc(sizeof(Dir));
		dir_open(resource_loaders[i].dir, resource_loaders[i].path, resource_loaders[i].loader, 1);
	}
	return 0;
}

int openlib_Resource(lua_State* default_lua) {
	L = default_lua;
	lua_settop(L, 0);
	lua_newtable(L);
	lua_setglobal(L, Resource_mt);
	lua_Resources_init(L);
	return 1;
}

/*
void resources_add(Resource* r) {
	int index = get_hash(resource->name, resource_table.size, 0);
	Resource* curr = &resource_table.resources[index];
	int i = 1;
	while (curr->type != NO_RES) {
		index = get_hash(resource->name, resource_table.size, i);
		curr = &resource_table.resources[index];
		i++;
	}
	resource_table.resources[index] = resource;
	resource_table.count++;
}

Resource *resources_get(const char* name) {
	int index = get_hash(name, resource_table.size, 0);
	Resource* curr = resource_table.resources[index];
	int i = 1;
	while (curr->type != NO_RES) {
		if (strcmp(curr->name, name) == 0) {
			return curr;
		}
		index = get_hash(name, resource_table.size, i);
		curr = resource_table.resources[index];
		i++;
	}
	printf("Resource not found: %s\n", name);
	return NULL;
}


void resources_clear() {
	Resource* r;
	for (int i = 0; i < resource_table.size; i++) {
		r = resource_table.resources[i];
		switch (r->type) {
		case TEXTURE:
			texture_delete((Texture*)r);
			break;
		case SHADER:
			shader_delete((Shader*)r);
			break;
		}
		free(r);
	}
	free(resource_table.resources);
}
*/