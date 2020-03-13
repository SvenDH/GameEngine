#pragma once
#include "types.h"
#include "data.h"
#include "resource.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#define NUM_BUFFERS 4
#define NUM_SOURCES 32
#define AUDIO_BUFFER_SIZE 65536 // 32kb of data in each buffer

typedef struct {
	ALuint ID;
	ALenum format;
	vec3 position, speed;
	float pitch, volume;	
	int offset, rate, buffered;
	char looping, channels, bits;
	struct {
		ALuint buffer[NUM_BUFFERS];
		char head, tail;
	} streaming;
	struct {
		ALuint buffer[NUM_BUFFERS];
		char head, tail;
	} available;
} source_t;

typedef struct {
	object_allocator_t sources;
	hashmap_t source_map;
	queue_t queue;
	hashmap_t playing;
	ALCdevice* device;
	ALCcontext* context;
	ALuint pool[NUM_SOURCES];
} audio_t;

void audio_init(audio_t* am, const char* name);
void audio_update(audio_t* am);
source_t* audio_newsource(audio_t* am, UID id, int rate, int bits, int channels);
void audio_deletesource(audio_t* am, UID id);
int audio_playsource(audio_t* am, source_t* source);
void audio_stopsource(audio_t* am, source_t* source);
void audio_pausesource(audio_t* am, source_t* source);

void source_init(source_t* source, int rate, int bits, int channels);
void source_delete(source_t* source);
int source_queue(source_t* source, void* data, size_t len);
int source_update(source_t* source);
void source_reset(source_t* source);

inline GLOBAL_SINGLETON(audio_t, audio, "Audio");