#include "file.h"
#include "memory.h"

static int dir_iter(lua_State *L);

int filesystem_cb(filesystem_t* fs, event_t evt) {
	assert(fs->current_request == evt.p1.ptr);
	fs->current_request = NULL;
	filesystem_poll(fs);
	return 0;
}

void filesystem_init(filesystem_t* filesystem, const char* name) {
	object_init(filesystem, name, sizeof(file_t), 16, 4);
	hashmap_init(&filesystem->files);
	object_init(&filesystem->requests, "FileRequest", sizeof(file_request), 16, 4);
	queue_init(&filesystem->queue, sizeof(file_request*));
	int size = MAX_PATH_LENGTH;
	uv_exepath(filesystem->exepath, &size);
	size = MAX_PATH_LENGTH;
	uv_os_homedir(filesystem->workpath, &size);
	printf("Save directory: %s", filesystem->workpath);
	filesystem->current_request = NULL;

	event_register(filesystem, fileread | filewrite, filesystem_cb, NULL);
}

file_t* file_new(filesystem_t* filesystem, const char* path) {
	assert(strlen(path) < MAX_PATH_LENGTH);
	file_t* file = hashmap_get(&filesystem->files, hash_string(path));
	if (!file) {
		file = object_alloc(filesystem, sizeof(file_t));
		file->fd = 0;
		file->open = 0;
		strcpy(file->path, path);
	}
	assert(strcmp(path, file->path) == 0);
	return file;
}

int file_open(file_t* file, const char* mode) {
	if (!file->open) { //TODO: change mode
		uv_fs_t req;
		int fd = uv_fs_open(uv_default_loop(), &req, file->path, str_to_mode(mode), 0644, NULL);
		if (fd >= 0) {
			//log_info("Opened file: %s", file->path);
			file->fd = req.result;
			file->open = 1;
		}
		else log_info("Could not open file: %s", file->path);
		uv_fs_req_cleanup(&req);
		return (fd >= 0);
	}
	return 0;
}

int file_size(file_t* file) {
	uv_fs_t req;
	uv_stat_t* stat = NULL;
	if (file->open && uv_fs_fstat(uv_default_loop(), &req, file->fd, NULL) == 0) {
		stat = uv_fs_get_statbuf(&req);
	}
	else if (uv_fs_stat(uv_default_loop(), &req, file->path, NULL) == 0) {
		stat = uv_fs_get_statbuf(&req); //TODO: fix this
	}
	return (stat!=NULL) ? stat->st_size : -1;
}

int file_read(file_t* file, char* data, int length, int offset) {
	uv_fs_t req;
	uv_buf_t buf = uv_buf_init(data, length);
	int isopen = file->open;
	if (!isopen) file_open(file, "r");
	int result = uv_fs_read(uv_default_loop(), &req, file->fd, &buf, 1, offset, NULL);
	uv_fs_req_cleanup(&req);
	if (!isopen) file_close(file);
	return result;
}
//TODO: code duplication
int file_write(file_t* file, char* data, int length, int offset) {
	uv_fs_t req;
	uv_buf_t buf = uv_buf_init(data, length);
	int isopen = file->open;
	if (!isopen) file_open(file, "w");
	int result = uv_fs_write(uv_default_loop(), &req, file->fd, &buf, 1, offset, NULL);
	uv_fs_req_cleanup(&req);
	if (!isopen) file_close(file);
	return result;
}

int file_flush(file_t* file) {
	uv_fs_t req;
	int result = uv_fs_fsync(uv_default_loop(), &req, file->fd, NULL);
	uv_fs_req_cleanup(&req);
	return result;
}

int file_close(file_t* file) {
	if (file->open) {
		uv_fs_t req;
		int result = uv_fs_close(uv_default_loop(), &req, file->fd, NULL);
		//log_info("Closed file: %s", file->path);
		uv_fs_req_cleanup(&req);
		file->open = 0;
		return 1;
	}
	return 0;
}

void file_delete(filesystem_t* filesystem, file_t* file) {
	if (file->open) file_close(file);
	hashmap_remove(&filesystem->files, hash_string(file->path));
	object_free(filesystem, file);
}


//TODO: priority parameter and maximum amount of file reads and writes at a time
file_request* request_new(file_t* file, req_type type, buffer_t* buffer, int length, int offset) {
	filesystem_t* fs = filesystem_instance();
	file_request* request = object_alloc(&fs->requests, sizeof(file_request));
	if (queue_put(&fs->queue, &request)) {
		request->file = file;
		request->type = type;
		request->buffer = buffer;
		request->length = length;
		request->offset = offset;
		request->status = FILE_BUSY;
		filesystem_poll(fs);
		return request;
	}
	object_free(&fs->requests, request);
	return NULL;
}

void request_release(file_request* request) {
	object_free(filesystem_instance(), request);
}

static void on_read_cb(uv_fs_t* req) {
	file_request* request = req->data;
	if (req->result >= 0) {
		request->status = FILE_DONE;
		bip_write(request->buffer, req->result);
	}
	else request->status = FILE_ERROR;
	request->length = req->result;
	uv_fs_req_cleanup(req);
	event_post((event_t) { 
		.type = on_fileread,
		.p0.ptr = request->file,
		.p1.ptr = request });
}

static void on_write_cb(uv_fs_t* req) {
	file_request* request = req->data;
	if (req->result >= 0) {
		request->status = FILE_DONE;
		bip_read(request->buffer, req->result);
	}
	else request->status = FILE_ERROR;
	request->length = req->result;
	uv_fs_req_cleanup(req);
	event_post((event_t) { 
		.type = on_filewrite,
		.p0.ptr = request->file,
		.p1.ptr = request });
}

void filesystem_poll(filesystem_t* fs) {
	if (!fs->current_request) {
		file_request* request;
		if (queue_get(&fs->queue, &request)) {
			char* data;
			switch (request->type) {
			case FILE_READ:
				data = bip_alloc(request->buffer, request->length);
				fs->buf = uv_buf_init(data, request->length);
				file_open(request->file, "r");
				uv_fs_read(uv_default_loop(), &fs->req, request->file->fd, &fs->buf, 1, request->offset, on_read_cb);
				break;
			case FILE_WRITE:
				data = bip_peek(request->buffer, request->length);
				fs->buf = uv_buf_init(data, request->length);
				file_open(request->file, "w");
				uv_fs_write(uv_default_loop(), &fs->req, request->file->fd, &fs->buf, 1, request->offset, on_write_cb);
				break;
			}
			fs->req.data = request;
			fs->current_request = request;
		}
	}
}

//File(path)
int w_file_new(lua_State* L) {
	char* path = luaL_checkstring(L, 2);
	file_t* file = file_new(filesystem_instance(), path);
	PUSH_LUA_POINTER(L, File_mt, file);
	return 1;
}

//File:open([mode])
int w_file_open(lua_State* L) {
	file_t* file = POP_LUA_POINTER(L, File_mt, 1, file_t);
	char* mode = luaL_optstring(L, 2, "r");
	if (!file_open(file, mode))
		luaL_error(L, "Could not open file");
	return 0;
}

//File:read([length, offset])
int w_file_read(lua_State* L) {
	file_t* file;
	const char* path;
	if (lua_type(L, 1) == LUA_TSTRING)
		file = file_new(filesystem_instance(), lua_tostring(L, 1));
	else 
		file = POP_LUA_POINTER(L, File_mt, 1, file_t);

	int length = luaL_optinteger(L, 2, file_size(file));
	int offset = luaL_optinteger(L, 3, 0);
	luaL_Buffer buff;
	char* data = luaL_buffinitsize(L, &buff, length);
	int result = file_read(file, data, length, offset);
	if (result >= 0)
		luaL_pushresultsize(&buff, result);
	else luaL_error(L, "Could not read file");
	return 1;
}

//File:write(data, [offset, length])
int w_file_write(lua_State* L) {
	file_t* file = POP_LUA_POINTER(L, File_mt, 1, file_t);
	int length;
	char* data = luaL_checklstring(L, 2, &length);
	int offset = luaL_optinteger(L, 3, -1);
	int result = file_write(file, data, length, offset);
	if (result < 0)
		luaL_error(L, "Could not write file");
	return 0;
}

//File:close()
int w_file_close(lua_State* L) {
	if (!file_close(POP_LUA_POINTER(L, File_mt, 1, file_t)))
		luaL_error(L, "Could not close file");
	return 0;
}

int w_file_gc(lua_State* L) {
	file_delete(filesystem_instance(), POP_LUA_POINTER(L, File_mt, 1, file_t));
}

//Dir(path)
int dir_list(lua_State* L) {
	const char* path = luaL_checkstring(L, 2);

	Dir *dir = (Dir *)lua_newuserdata(L, sizeof(Dir));
	luaL_setmetatable(L, Dir_mt);
	int err = uv_fs_scandir(uv_default_loop(), &dir->req, path, 0, NULL);
	if (err < 0) luaL_error(L, "error cannot open dir %s: %s", path, uv_strerror(err));

	lua_pushcclosure(L, dir_iter, 1);
	return 1;
}

int dir_iter(lua_State *L) {
	Dir *dir = (Dir *)lua_touserdata(L, lua_upvalueindex(1));
	uv_dirent_t ent;
	while (uv_fs_scandir_next(&dir->req, &ent) != UV_EOF) {
		if (ent.type == UV_DIRENT_FILE) {
			lua_pushstring(L, ent.name);
			return 1;
		}
	}
	return 0;
}

//dir:__gc()
int dir_gc(lua_State *L) {
	Dir *dir = (Dir *)lua_touserdata(L, 1);
	if (dir) uv_fs_req_cleanup(&dir->req);
	return 0;
}

int openlib_File(lua_State* L) {
	static luaL_Reg file_func[] = {
		{"open", w_file_open},
		{"read", w_file_read},
		{"write", w_file_write},
		{"close", w_file_close},
		{NULL, NULL},
	};

	static luaL_Reg dir_func[] = {
			{"__gc", dir_gc},
			{NULL, NULL},
	};

	create_lua_class(L, File_mt, w_file_new, file_func);
	create_lua_class(L, Dir_mt, dir_list, dir_func);

	return 0;
}