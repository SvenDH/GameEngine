#include "file.h"
#include "data.h"

static int dir_iter(lua_State *L);
static void on_read_cb(uv_fs_t* req);

//file(path [, mode])
static int file_open(lua_State* L) {
	int top = lua_gettop(L);
	File *file;
	char* mode;
	char* path = luaL_checkstring(L, 1);
	if (top > 1) mode = luaL_checkstring(L, 2);
	// TODO: implement mode
	uv_fs_t req;
	int fd = uv_fs_open(uv_default_loop(), &req, path, O_RDWR, 0644, NULL);
	if (fd > 0) {
		file = (File*)lua_newuserdata(L, sizeof(File));
		luaL_setmetatable(L, File_mt);
		file->fd = req.result;
	}
	else {
		luaL_error(L, "error opening file: %s\n", uv_strerror((int)req.result));
		lua_pushnil(L);
	}
	return 1;
}

//File:read([callback])
static int file_read(lua_State* L) {
	int cb = -1;
	int top = lua_gettop(L);

	File *file = (File*)luaL_checkudata(L, 1, File_mt);
	if (top > 1) cb = luaL_ref(L, LUA_REGISTRYINDEX);

	file->req = malloc(sizeof(uv_fs_t));
	file->req->data = file;
	PoolBuffer* buf = buffer_alloc();
	file->buf = uv_buf_init(buf->data, MAX_BUFF_SIZE);

	if (cb > -1) {
		file->callstate = L;
		file->callback = cb;
		uv_fs_read(uv_default_loop(), file->req, file->fd, &file->buf, 1, -1, on_read_cb);
	}
	else {
		int result = uv_fs_read(uv_default_loop(), file->req, file->fd, &file->buf, 1, -1, NULL);
		if (result < 0) {
			luaL_error(L, "error reading file: %s\n", uv_strerror(result));
		}
		else {
			lua_pushlstring(L, buf->data, result);
			return 1;
		}
	}
	return 0;
}

//File:close()
static int file_close(lua_State* L) {
	File *file = (File*)luaL_checkudata(L, 1, File_mt);
	uv_fs_t req;
	uv_fs_close(uv_default_loop(), &req, file->fd, NULL);
	uv_fs_req_cleanup(&req);
	return 0;
}

//function(err, data)
static void on_read_cb(uv_fs_t* req) {
	File* file = (File*)req->data;
	if (req->result < 0) luaL_error(file->callstate, "error reading file: %s\n", uv_strerror(req->result));
	PoolBuffer* buf = (PoolBuffer*)file->buf.base;
	buf->len = req->result;
	lua_rawgeti(file->callstate, LUA_REGISTRYINDEX, file->callback);
	lua_pushinteger(file->callstate, req->result);
	lua_pushlstring(file->callstate, buf->data, buf->len);
	lua_call(file->callstate, 1, 0);

	luaL_unref(file->callstate, LUA_REGISTRYINDEX, file->callback);
	uv_fs_req_cleanup(req);
	free(req);
}

//dir.list(path)
static int dir_list(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);

	Dir *dir = (Dir *)lua_newuserdata(L, sizeof(Dir));
	dir->req = malloc(sizeof(uv_fs_t));

	luaL_getmetatable(L, Dir_mt);
	lua_setmetatable(L, -2);

	int err = uv_fs_scandir(uv_default_loop(), dir->req, path, 0, NULL);
	if (err < 0) luaL_error(L, "error cannot open dir %s: %s", path, uv_strerror(err));

	lua_pushcclosure(L, dir_iter, 1);
	return 1;
}

//dir.iter
static int dir_iter(lua_State *L) {
	Dir *dir = (Dir *)lua_touserdata(L, lua_upvalueindex(1));
	uv_dirent_t ent;
	while (uv_fs_scandir_next(dir->req, &ent) != UV_EOF) {
		if (ent.type == UV_DIRENT_FILE) {
			lua_pushstring(L, ent.name);
			return 1;
		}
	}
	return 0;
}

//dir.__gc
static int dir_gc(lua_State *L) {
	Dir *dir = (Dir *)lua_touserdata(L, 1);
	if (dir) uv_fs_req_cleanup(dir->req);
	return 0;
}

static luaL_Reg file_func[] = {
		{"read", file_read},
		{"close", file_close},
		{NULL, NULL},
};

static luaL_Reg dir_func[] = {
		{"__gc", dir_gc},
		{NULL, NULL},
};

int openlib_File(lua_State* L) {
	luaL_newmetatable(L, File_mt);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, file_func, 0);
	lua_pop(L, 1);

	lua_pushcfunction(L, file_open);
	lua_setglobal(L, "file");

	luaL_newmetatable(L, Dir_mt);
	luaL_setfuncs(L, dir_func, 0);
	lua_pop(L, 1);

	lua_pushcfunction(L, dir_list);
	lua_setglobal(L, "dir");

	return 0;
}