#include "audio.h"

void audio_init(audio_t* am, const char* name) {
	object_init(&am->sources, name, sizeof(source_t), 16, 8);
	hashmap_init(&am->source_map);
	queue_init(&am->id, sizeof(ALuint));
	hashmap_init(&am->playing);

	am->device = alcOpenDevice(NULL); 
	if (am->device == NULL) { 
		printf("Failed to init OpenAL device."); 
		return; 
	}
	am->context = alcCreateContext(am->device, NULL); 
	alcMakeContextCurrent(am->context);
	al_check_error();
	//Create sources and make available
	alGenSources(NUM_SOURCES, am->pool);
	for (int i = 0; i < NUM_SOURCES; i++)
		queue_put(&am->id, &am->pool[i]);
}

void audio_update(audio_t* am) {
	source_t* source; ALuint id;
	hashmap_foreach(&am->playing, source, id)
		if (!source_update(source))
			audio_stopsource(am, source);
}

source_t* audio_newsource(audio_t* am, UID id, int rate, int bits, int channels) {
	source_t* source = object_alloc(&am->sources, sizeof(source_t)); 
	source_init(source, rate, bits, channels);
	hashmap_put(&am->source_map, id, source);
	return source;
}

void audio_deletesource(audio_t* am, UID id) {
	source_t* source = hashmap_get(&am->source_map, id);
	assert(source);
	audio_stopsource(am, source);
	source_delete(source);
	hashmap_remove(&am->source_map, id);
}

int audio_playsource(audio_t* am, source_t* source) {
	ALuint id = hashmap_get(&am->playing, source);
	if (!id) {
		if(!queue_get(&am->id, &id))
			return 0;

		source->ID = id;
		source_reset(source);
		while (!circbuffer_empty(&source->streaming, NUM_BUFFERS)) {
			ALuint buff; //TODO: can we use bipbuffer?
			circbuffer_pop(&source->streaming, buff, NUM_BUFFERS);
			alSourceQueueBuffers(id, 1, &buff);
		}
		alSourcei(id, AL_SAMPLE_OFFSET, source->offset);
		alSourcePlay(id);
		source->offset = 0;
		hashmap_put(&am->playing, source, id);
	}
	else {
		ALenum state;
		alGetSourcei(source->ID, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) alSourcePlay(id);
	}
	return 1;
}

void audio_stopsource(audio_t* am, source_t* source) {
	if (!source->ID) return;
	ALuint id = hashmap_get(&am->playing, source);
	if (id) {
		alSourceStop(id);
		ALint queued;
		ALuint buffers[NUM_BUFFERS];
		alGetSourcei(id, AL_BUFFERS_QUEUED, &queued);
		alSourceUnqueueBuffers(id, queued, buffers);
		for (int i = 0; i < queued; i++)
			circbuffer_push(&source->available, buffers[i], NUM_BUFFERS);
		alSourcei(id, AL_BUFFER, AL_NONE);
		source->ID = 0;
		source->looping = 0;
		source->offset = 0;
		hashmap_remove(&am->playing, source);
		queue_put(&am->id, &id);
	}
}

void audio_pausesource(audio_t* am, source_t* source) {
	ALuint id = hashmap_get(&am->playing, source);
	if (id) alSourcePause(id);
}