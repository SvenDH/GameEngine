#include "resource.h"
#include "utils.h"

#define RESOURCE_SCRATCH_SIZE 16384 //16kb
#define RESOURCE_BUFFER_SIZE 4194304 //4mb

int resource_file_cb(resourcemanager_t* rm, Event evt) {
	file_request* req = (file_request*)evt.data;
	resource_t* res = hashmap_get(&rm->requests, req);
	if (res) {
		if (req->status == FILE_DONE) { //TODO: handle file error
			size_t length = req->length;
			char* data = bip_peek(&rm->buffer, length);
			//rm->loaders[res->uid.type](rm, res, data, length);
			bip_read(&rm->buffer, length);
		}
		hashmap_remove(&rm->requests, req);
		request_release(req);
	}
	return 0;
}

void resourcemanager_init(resourcemanager_t* manager, const char* name) {
	objectallocator_init(manager, name, sizeof(res_union), 8, 8);
	hashmap_init(&manager->resources);
	hashmap_init(&manager->requests);
	bip_init(&manager->buffer, memoryuser_alloc(manager, RESOURCE_BUFFER_SIZE, PAGE_SIZE), RESOURCE_BUFFER_SIZE);
	event_register(eventhandler_instance(), manager, EVT_FILEREAD, resource_file_cb);
}

void resource_load(resourcemanager_t* rm, resource_t* res, const char* path) {
	char data[RESOURCE_BUFFER_SIZE];
	file_t* file = file_new(filesystem_instance(), path);
	size_t len = file_size(file);
	assert(len < RESOURCE_BUFFER_SIZE);
	file_read(file, data, len, 0);
	data[len] = '\0';
	//rm->loaders[res->uid.type](rm, res, data, len);
}

void resource_loadasync(resourcemanager_t* rm, resource_t* res, const char* path) {
	file_t* file = file_new(filesystem_instance(), path);
	file_request* request = async_file_read(
		file,
		&rm->buffer,
		file_size(file),
		0);
	hashmap_put(&rm->requests, request, res);
}

 resource_t* resource_new(resourcemanager_t* rm, uint32_t name, int type, intptr_t load) {
	assert(type < RES_MAX);
	rid_t uid = { .name = name,.type = type };
	resource_t* res = objectallocator_alloc(rm, sizeof(res_union));
	res->load = load;
	res->loaded = 0;
	hashmap_put(&rm->resources, uid.value, res);
	return res;
}

 resource_t* resource_get(resourcemanager_t* rm, rid_t uid) {
	 return hashmap_get(&rm->resources, uid.value);
 }

void resource_release(resourcemanager_t* rm, rid_t uid) {
	resource_t* res = hashmap_get(&rm->resources, uid.value);
	if (res) {
		//rm->unloaders[res->uid.type](rm, res);
		hashmap_remove(&rm->resources, uid.value);
		objectallocator_free(rm, res);
	}
}

//Resource(name, type)
int w_resource_new(lua_State* L) {
	resourcemanager_t* rm = resourcemanager_instance();
	rid_t uid;
	char* path = NULL;
	uid.type = luaL_checkinteger(L, 3);
	switch (lua_type(L, 2)) {
	case LUA_TSTRING:
		path = luaL_checkstring(L, 2);
		uid.name = hash_string(path);
		break;
	case LUA_TTABLE:
		uid.name = hash_luatable(L, 2);
	}
	//log_info("0x%llx", uid.value);
	resource_t* res = hashmap_get(&rm->resources, uid.value);
	if (!res) {
		lua_pushvalue(L, 2); //Refer input params
		res = resource_new(rm, uid.name, uid.type, luaL_ref(L, LUA_REGISTRYINDEX));
	}
	rid_t* ref = lua_newuserdata(L, sizeof(rid_t));
	luaL_setmetatable(L, res_mt[uid.type]);
	*ref = uid;
	return 1;
}

//Resource:load()
int w_resource_load(lua_State* L) { //TODO: supply other path/table
	resource_t* res = resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	if (res && !res->loaded) {
		res->loaded = 1;
		luaL_getmetafield(L, 1, "__load");
		lua_pushvalue(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, res->load);
		//If string we assume path, else table
		if (lua_type(L, -1) == LUA_TSTRING) {
			char* path = lua_tostring(L, -1);
			luaL_Buffer buff;
			file_t* file = file_new(filesystem_instance(), path);
			size_t len = file_size(file);
			char* data = luaL_buffinitsize(L, &buff, len);
			int result = file_read(file, data, len, 0);
			if (result >= 0) luaL_pushresultsize(&buff, result);
			else luaL_error(L, "Could not read file");
			lua_remove(L, -2); //Remove path
		}
		lua_call(L, 2, 0);
	}
	return 1;
}

//Resource:unload()
int w_resource_unload(lua_State* L) {
	resource_t* res = resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	if (res && res->loaded) {
		res->loaded = 0;
		luaL_getmetafield(L, 1, "__unload");
		lua_pushvalue(L, 1);
		lua_call(L, 1, 0);
	}
	return 1;
}

//Resource.__tostring()
int w_resource_tostring(lua_State* L) {
	rid_t* name = (rid_t*)lua_touserdata(L, 1);
	char buff[16];
	sprintf(buff, "0x%llx", *name);
	lua_pushlstring(L, buff, 16);
	return 1;
}

int openlib_Resources(lua_State* L) {
	static luaL_Reg image_func[] = {
		{"load", w_resource_load},
		{"__load", w_image_load},
		{"__unload", w_image_unload},
		{NULL, NULL}
	};

	static luaL_Reg script_func[] = {
		{"load", w_resource_load},
		{"__load", w_script_load},
		{"__unload", w_script_unload},
		{"new", w_script_run},
		{NULL, NULL}
	};

	static luaL_Reg texture_func[] = {
		{"load", w_resource_load},
		{"__load", w_texture_load},
		{"__unload", w_texture_unload},
		{"__index", w_texture_index},
		{"draw", w_texture_draw},
		{NULL, NULL}
	};

	static luaL_Reg shader_func[] = {
		{"load", w_resource_load},
		{"__load", w_shader_load},
		{"__unload", w_shader_unload},
		{NULL, NULL}
	};

#define X(num, name) create_lua_class(L, #name, w_##name##_new, name##_func);
	RESOURCE_DATA
#undef X

	static luaL_Reg resource_func[] = {
		{"__tostring", w_resource_tostring},
		{"load", w_resource_load},
		{"unload", w_resource_unload},
		{NULL, NULL}
	};
	create_lua_class(L, Resource_mt, w_resource_new, resource_func);
}