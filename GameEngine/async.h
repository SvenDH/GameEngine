#pragma once
#include "data.h"
#include <uv.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PATH_LEN 1024

typedef void(*Callback) ();

typedef struct Timer {
	uv_timer_t handle;
	Callback callback;
	int repeat;
} Timer;

void timer_init(Timer* timer, Callback c, int timeout, int repeat);

typedef struct File {
	uv_file fd;
	uv_fs_t* req;
	uv_buf_t buf;
	uv_fs_event_t eventhandle;
	Callback callback;
	const char* path;
	const char* name;
	void* data;
} File;

void file_open(File* file, Callback c, const char* filename, const char* path, int watch);
void file_read(File* file, Callback c, Buffer* b);
void file_close(File* file);

typedef struct Dir {
	File* files;
	size_t size;
	uv_fs_t* req;
	Callback callback;
	int watch;
	const char* path;
} Dir;

void dir_open(Dir* dir, const char* path, Callback c, int watch);

inline void async_run() {
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

inline void async_stop() {
	//uv_stop(uv_default_loop());
}