#pragma once
#include "data.h"
#include "platform.h"
#include "event.h"
#include "utils.h"

#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define File_mt "File"
#define Dir_mt "Dir"

#define async_file_read(_file, _data, _length, _offset) \
	request_new((_file), FILE_READ, (_data), (_length), (_offset))

#define async_file_write(_file, _data, _length, _offset) \
	request_new((_file), FILE_WRITE, (_data), (_length), (_offset))

typedef struct {
	uv_file fd;
	int open;
	char path[MAX_PATH_LENGTH];
} file_t;

typedef enum {
	FILE_ERROR = -1,
	FILE_BUSY = 0,
	FILE_DONE = 1,
} file_status;

typedef enum {
	FILE_READ,
	FILE_WRITE,
} req_type;

typedef struct {
	file_t* file;
	buffer_t* buffer;
	req_type type;
	int offset;
	int length;
	int status;
} file_request;

typedef struct {
	ObjectAllocator;
	hashmap_t files;
	ObjectAllocator requests;
	file_request* current_request;
	queue_t queue;
	uv_fs_t req;
	uv_buf_t buf;
	char exepath[MAX_PATH_LENGTH];
	char workpath[MAX_PATH_LENGTH];
} filesystem_t;

typedef struct Dir {
	uv_fs_t req;
} Dir;

void filesystem_init(filesystem_t* filesystem, const char* name);
inline GLOBAL_SINGLETON(filesystem_t, filesystem, "File");
void filesystem_poll(filesystem_t* filesystem);
file_t* file_new(filesystem_t* filesystem, const char* path);
int file_open(file_t* file, const char* mode);
int file_size(file_t* file);
int file_read(file_t* file, void* data, int length, int offset);
int file_write(file_t* file, void* data, int length, int offset);
int file_flush(file_t* file);
int file_close(file_t* file);
void file_delete(filesystem_t* filesystem, file_t* file);

file_request* request_new(file_t* file, req_type type, buffer_t* buffer, int length, int offset);
void request_release(file_request* req);

int w_file_new(lua_State* L);
int w_file_open(lua_State* L);
int w_file_read(lua_State* L);
int w_file_write(lua_State* L);
int w_file_close(lua_State* L);
int w_file_gc(lua_State* L);
int dir_list(lua_State* L);
int dir_iter(lua_State *L);
int dir_gc(lua_State *L);

int openlib_File(lua_State* L);