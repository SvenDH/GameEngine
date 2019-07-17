#include "file.h"
#include "data.h"
#include "utils.h"

static int dir_iter(lua_State *L);
static void on_read_cb(uv_fs_t* req);

//File(path [, mode])
static int file_open(lua_State* L) {
	File *file;
	uv_fs_t req;
	char* path = luaL_checkstring(L, 2);
	char* mode = luaL_optstring(L, 3, "r");
	int flags = str_to_mode(mode);
	// TODO: implement mode
	log_info("Opened file: %s", path);
	int fd = uv_fs_open(uv_default_loop(), &req, path, flags, 0644, NULL);
	if (fd >= 0) {
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
	file->buf = uv_buf_init((char*)malloc(MAX_BUFF_SIZE), MAX_BUFF_SIZE);

	if (cb > -1) {
		file->callstate = L;
		file->callback = cb;
		uv_fs_read(uv_default_loop(), file->req, file->fd, &file->buf, 1, -1, on_read_cb);
	}
	else {
		int result = uv_fs_read(uv_default_loop(), file->req, file->fd, &file->buf, 1, -1, NULL);
		if (result <= 0) {
			luaL_error(L, "error reading file: %s\n", uv_strerror(result));
		}
		else {
			lua_pushlstring(L, file->buf.base, result);
			free(file->buf.base);
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
	lua_rawgeti(file->callstate, LUA_REGISTRYINDEX, file->callback);
	lua_pushlstring(file->callstate, file->buf.base, req->result);
	lua_call(file->callstate, 1, 0);

	luaL_unref(file->callstate, LUA_REGISTRYINDEX, file->callback);
	uv_fs_req_cleanup(req);
	free(req);
}

//Dir(path)
static int dir_list(lua_State* L) {
	const char* path = luaL_checkstring(L, 2);

	Dir *dir = (Dir *)lua_newuserdata(L, sizeof(Dir));
	dir->req = malloc(sizeof(uv_fs_t));

	luaL_setmetatable(L, Dir_mt);

	int err = uv_fs_scandir(uv_default_loop(), dir->req, path, 0, NULL);
	if (err < 0) luaL_error(L, "error cannot open dir %s: %s", path, uv_strerror(err));

	lua_pushcclosure(L, dir_iter, 1);
	return 1;
}

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

//dir:__gc()
static int dir_gc(lua_State *L) {
	Dir *dir = (Dir *)lua_touserdata(L, 1);
	if (dir) uv_fs_req_cleanup(dir->req);
	return 0;
}

int openlib_File(lua_State* L) {
	static luaL_Reg file_func[] = {
		{"read", file_read},
		{"close", file_close},
		{NULL, NULL},
	};

	static luaL_Reg dir_func[] = {
			{"__gc", dir_gc},
			{NULL, NULL},
	};

	create_lua_class(L, File_mt, file_open, file_func);
	create_lua_class(L, Dir_mt, dir_list, dir_func);

	return 0;
}