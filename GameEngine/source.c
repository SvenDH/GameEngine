#include "audio.h"
#include "resource.h"
#include "utils.h"

void source_init(source_t* source, int rate, int bits, int channels) {
	memset(source, 0, sizeof(source_t));
	switch (channels) {
	case 1:
		if (bits == 8)
			source->format = AL_FORMAT_MONO8;
		else if (bits == 16)
			source->format = AL_FORMAT_MONO16;
		break;
	case 2:
		if (bits == 8)
			source->format = AL_FORMAT_STEREO8;
		else if (bits == 16)
			source->format = AL_FORMAT_STEREO16;
		break;
	}
	source->channels = channels;
	source->bits = bits;
	source->rate = rate;
	source->looping = 1;
	source->pitch = 1.0f;
	source->volume = 1.0;
	memset(source->position, 0, sizeof(vec3));
	memset(source->speed, 0, sizeof(vec3));
	alGenBuffers(NUM_BUFFERS, source->available.buffer);
	//Set head to and of buffer
	source->available.head = NUM_BUFFERS - 1;
}

void source_delete(source_t* source) {
	alDeleteBuffers(1, &source->available.buffer);
}

int source_queue(source_t* source, void* data, size_t len) {
	if (len == 0) 
		return 1;
	if (circbuffer_empty(&source->available, NUM_BUFFERS))
		return 0;

	ALuint id;
	circbuffer_pop(&source->available, id, NUM_BUFFERS);
	alBufferData(id, source->format, data, len, source->rate);
	source->buffered += len;
	if (source->ID)
		alSourceQueueBuffers(source->ID, 1, &id);
	else
		circbuffer_push(&source->streaming, id, NUM_BUFFERS);

	return 1;
}

int source_update(source_t* source) {
	if (!source->ID) 
		return 0;

	ALint processed;
	ALuint buffers[NUM_BUFFERS];
	alGetSourcei(source->ID, AL_BUFFERS_PROCESSED, &processed);
	alSourceUnqueueBuffers(source->ID, processed, buffers);
	for (int i = 0; i < processed; i++) {
		ALint size;
		alGetBufferi(buffers[i], AL_SIZE, &size);
		source->buffered -= size;
		circbuffer_push(&source->available, buffers[i], NUM_BUFFERS);
	}
	ALenum state;
	alGetSourcei(source->ID, AL_SOURCE_STATE, &state);
	return (state != AL_STOPPED);
}

void source_reset(source_t* source) {
	alSourcei(source->ID, AL_BUFFER, AL_NONE);
	alSourcefv(source->ID, AL_POSITION, source->position);
	alSourcefv(source->ID, AL_VELOCITY, source->speed);
	alSourcef(source->ID, AL_PITCH, source->pitch);
	alSourcef(source->ID, AL_GAIN, source->volume);
	alSourcei(source->ID, AL_LOOPING, source->looping);
}
