#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"
#include "shader.h"
#include "texture.h"
#include "script.h"
#include "async.h"

#define MAX_RESOURCES 149

#define Resource_mt "Resource"

#define RESOURCE_INIT(_r, _name, _type) \
	(_r) = malloc(sizeof(Resource)); \
	(_r)->params = tok_to_list(_name, "."); \
	(_r)->name = (_r)->params[0]; \
	(_r)->type = _type; \
	resources_add((_r))

typedef enum resource_t {
	NO_RES = 0,
	TEXTURE,
	SHADER,
	SCRIPT,
	MAX_RES_TYPES
} resource_t;

typedef struct ResourceLoader {
	Dir* dir;
	char* path;
	void (*loader)();
} ResourceLoader;

typedef struct Resource {
	union {
		struct Texture;
		struct Shader;
		struct Script;
	};
	resource_t type;
	char* name;
	char** params;
} Resource;

typedef struct ResourceTable {
	int size;
	int count;
	Resource** resources;
} ResourceTable;

void resources_add(Resource* resource);
Resource *resources_get(const char* name);

int openlib_Resource(lua_State* L);