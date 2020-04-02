#pragma once
#include "data.h"
#include "file.h"

#define Resource_mt "resource"

//TODO: check for null resource
#define NULL_RES (rid_t){ .value = 0 }

#define LOAD_RESOURCE(_L, _i) \
do { \
	luaL_getmetafield((_L), (_i), "load"); \
	lua_pushvalue((_L), (_i)); \
	lua_call((_L), 1, 0); \
} while (0)

typedef union {
	struct {
		uint32_t name;
		uint32_t type;
	};
	uint64_t value;
} rid_t;

typedef struct {
//	rid_t uid;
	intptr_t load;
	int loaded;
} resource_t;

typedef struct {
	resource_t;
	int width, height, channels;
	char* data;
} image_t;

typedef struct {
	resource_t;
	lua_State* L;
	int ref;
} script_t;

typedef struct {
	resource_t;
	uint id;
	int format;
	int width, height, depth;
} texture_t;

typedef struct {
	resource_t;
	uint id;
	int staged;
} shader_t;

typedef struct {
	resource_t;
	int length, rate, channels, bits;
	const char* data;
} sound_t;

typedef struct space_t;

#define RESOURCE_DATA \
	X(IMAGE, image) \
	X(SCRIPT, script) \
	X(TEXTURE, texture) \
	X(SHADER, shader) \
	X(SOUND, sound)

enum resource_enum {
#define X(num, name) RES_##num,
	RESOURCE_DATA
#undef X
	RES_MAX
};

static char* res_mt[] = {
#define X(num, name) #name,
	RESOURCE_DATA
#undef X
};

typedef union {
#define X(num, name) name##_t name;
	RESOURCE_DATA
#undef X
} res_union;

typedef struct {
	object_allocator_t;
	hashmap_t resources;
	hashmap_t requests;
	buffer_t buffer;
} resourcemanager_t;

void resourcemanager_init(resourcemanager_t* manager, const char* name);
resource_t* resourcemanager_lookup(resourcemanager_t* rm, rid_t uid);

rid_t resource_new(uint32_t name, int type, intptr_t load);
void resource_load(resource_t* res, const char* path);
bool resource_isloaded(rid_t uid);
void resource_release(rid_t uid);
resource_t* resource_get(rid_t uid);

void image_load(image_t* image, const char* data, size_t len);
void image_unload(image_t* image);

void sound_load(sound_t* sound, const char* data, size_t len);
void sound_unload(sound_t* sound);

void script_load(script_t* script, lua_State* L, const char* data, size_t len);
void script_unload(script_t* script);
int script_run(script_t* script);

int w_resource_new(lua_State* L);
int w_resource_load(lua_State* L);
int w_resource_unload(lua_State* L);
int w_resource_tostring(lua_State* L);

#define X(num, name) \
int w_##name##_new(lua_State* L); \
int w_##name##_load(lua_State* L); \
int w_##name##_unload(lua_State* L);
RESOURCE_DATA
#undef X

int w_script_run(lua_State* L);

int w_texture_index(lua_State* L);
int w_texture_draw(lua_State* L);

int openlib_Resources(lua_State* L);

inline GLOBAL_SINGLETON(resourcemanager_t, resourcemanager, "Resources");