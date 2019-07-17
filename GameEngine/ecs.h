#pragma once
#include "platform.h"
#include "event.h"
#include "entity.h"
#include "component.h"
#include "data.h"
#include "utils.h"

#include <lauxlib.h>

#define Entity_mt		"Entity"
#define Chunk_mt		"Chunk"
#define World_mt		"World"

#define MAX_WORLDS 16
#define MAX_COMPS 16
#define MAX_CHUNKS 256
#define CHUNK_SIZE 16384

#define NULL_ENTITY ((entity_handle)0)

typedef union {
	struct {
		uint16_t index;
		uint8_t chunk;
		uint8_t world;
	};
	void* ptr;
} entity_handle;

typedef enum {
	CHUNK_NONE = 0,
	CHUNK_LOADING = 1,
	CHUNK_LOADED = 2,
} chunk_state;

struct Chunk;

typedef struct Chunk {
	int16_t id;
	union {
		struct {
			uint32_t world;
			uint32_t x;
			uint32_t y;
		};
		uint8_t key[12];
	};									//14
	uint32_t state;						//18
	uint32_t ent_count;					//22

	char data[4];

} Chunk;

typedef struct World {
	int id;
	int chunkcount;
	int max_ents;
	int component_mask;
	int nr_components;
	uint8_t sizes[MAX_COMPS];
	uint16_t offsets[MAX_COMPS];
	Chunk* chunks[MAX_CHUNKS];
} World;

Chunk* chunk_new(int world, int x, int y);
Chunk* chunk_get(int world, int x, int y);
void chunk_delete(Chunk* chunk);

World* world_init(int nr_entities, int* sizes, int nr_components);
World* world_get(int id);
void world_delete(World* world);

void* chunk_get_componentarray(Chunk* chunk, int type);
void* component_get(entity_handle components, int type);

int openlib_ECS(lua_State* L);