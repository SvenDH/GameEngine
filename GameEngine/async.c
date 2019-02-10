#include "async.h"
#include "utils.h"

static void timer_cb(uv_timer_t* handle) {
	((Timer*)handle)->callback();
}

static void on_file_event_cb(uv_fs_event_t* handle, const char* name, int events, int status) {
	const char path[MAX_PATH_LEN]; size_t path_len;
	if (uv_fs_event_getpath(handle, path, &path_len))
		((File*)handle->data)->callback((File*)handle->data);
}

static void on_open_cb(uv_fs_t* req) {
	File* file = (File*)req->data;
	file->fd = req->result;
	if (req->result >= 0) file->callback(file);
	else fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
	uv_fs_req_cleanup(req);
	free(req);
}

static void on_read_cb(uv_fs_t* req) {
	File* file = (File*)req->data;
	if (req->result >= 0) {
		((Buffer*)file->buf.base)->len = req->result;
		file->callback(file);
	}
	else fprintf(stderr, "error reading file: %s\n", uv_strerror(req->result));
	uv_fs_req_cleanup(req);
	free(req);
}

static void dir_open_cb(uv_fs_t* req) {
	uv_dirent_t ent;
	Dir* d = (Dir*)req->data;
	d->files = malloc(1024 * sizeof(File));
	d->size = 0;
	while (uv_fs_scandir_next(req, &ent) != UV_EOF)
		if (ent.type == UV_DIRENT_FILE)
			file_open(&d->files[d->size++], d->callback, ent.name, d->path, d->watch);
	
	d->files = realloc(d->files, d->size * sizeof(File));
	uv_fs_req_cleanup(req);
	free(req);
}


void timer_init(Timer* timer, Callback c, int timeout, int repeat) {
	timer->callback = c;
	timer->repeat = repeat;
	uv_timer_init(uv_default_loop(), (uv_timer_t*)timer);
	uv_timer_start((uv_timer_t*)timer, timer_cb, timeout, repeat);
}

void file_open(File* file, Callback c, const char* filename, const char* path, int watch) {
	char temp[MAX_PATH_LEN]; sprintf(temp, "%s/%s", path, filename);
	file->path = strdup(path); file->name = strdup(filename);
	file->callback = c;
	file->req = malloc(sizeof(uv_fs_t));
	file->req->data = file;
	uv_fs_open(uv_default_loop(), file->req, temp, O_CREAT | O_RDWR, 0644, on_open_cb);
	if (watch) {
		file->eventhandle.data = file;
		uv_fs_event_init(uv_default_loop(), &file->eventhandle);
		uv_fs_event_start(&file->eventhandle, on_file_event_cb, file->path, 0);
	}
}

void file_read(File* file, Callback c, Buffer* b) {
	file->buf = uv_buf_init(b->data, MAX_BUFF_SIZE);
	file->callback = c;
	file->req = malloc(sizeof(uv_fs_t));
	file->req->data = file;
	uv_fs_read(uv_default_loop(), file->req, file->fd, &file->buf, 1, -1, on_read_cb);
}

void file_close(File* file) {
	uv_fs_t req;
	uv_fs_close(uv_default_loop(), &req, file->fd, NULL);
	uv_fs_req_cleanup(&req);
}

void dir_open(Dir* dir, const char* path, Callback c, int watch) {
	dir->watch = watch;
	dir->callback = c;
	dir->path = strdup(path);
	dir->req = malloc(sizeof(uv_fs_t));
	dir->req->data = dir;
	uv_fs_scandir(uv_default_loop(), dir->req, path, 0, dir_open_cb);
}