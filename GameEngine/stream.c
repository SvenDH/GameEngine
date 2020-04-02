#include "graphics.h"

void streambuffer_init(StreamBuffer* buffer, size_t size) {
	glGenBuffers(1, &buffer->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);

	GLbitfield storageflags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	GLbitfield mapflags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	glBufferStorage(GL_ARRAY_BUFFER, size * BUFFER_FRAMES, NULL, storageflags);

	buffer->data = (char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, size * BUFFER_FRAMES, mapflags);
	buffer->size = size;
	buffer->offset = 0;
	buffer->index = 0;

	for (int i = 0; i < BUFFER_FRAMES; i++)
		buffer->syncs[i] = 0;
}

void* streambuffer_map(StreamBuffer* buffer) {
	GLsync sync = buffer->syncs[buffer->index];
	GLbitfield flags = 0;
	GLuint64 duration = 0;
	while (sync) {
		GLenum status = glClientWaitSync(sync, flags, duration);
		if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED || status == GL_WAIT_FAILED)
			break;

		flags = GL_SYNC_FLUSH_COMMANDS_BIT;
		duration = 1000000000; // 1 second in nanoseconds.
	}
	glDeleteSync(sync);
	buffer->syncs[buffer->index] = 0;

	return buffer->data + (buffer->index * buffer->size) + buffer->offset;
}

size_t streambuffer_unmap(StreamBuffer* buffer, size_t used) {
	size_t offset = (buffer->index * buffer->size) + buffer->offset;
	buffer->offset += used;
	return offset;
}

void streambuffer_nextframe(StreamBuffer* buffer) {
	GLsync sync = buffer->syncs[buffer->index];
	if (sync) glDeleteSync(sync);
	buffer->syncs[buffer->index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	buffer->index = (buffer->index + 1) % BUFFER_FRAMES;
	buffer->offset = 0;
}