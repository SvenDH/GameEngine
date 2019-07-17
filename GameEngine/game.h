#pragma once
#include "texture.h"
#include "utils.h"
#include "data.h"
#include "component.h"

#include <stdint.h>
#include <lauxlib.h>

#define Chunk_mt "Chunk"
#define World_mt "World"

#define TW 16
#define TH 16
#define CW 32
#define CH 32

#define MAX_WORLDS 4
#define MAX_COMPS 16
#define CHUNK_SIZE 65536
#define MAX_ENT 512
#define NR_TILES CW*CH

typedef enum {
	CHUNK_NONE = 0,
	CHUNK_LOADING = 1,
	CHUNK_LOADED = 2,
} chunk_state;

struct Chunk;

union chunk_handle {
	struct {
		uint32_t world;
		uint32_t x;
		uint32_t y;
	};
	uint8_t key[12];
};

struct chunk_node {
	struct Chunk* prev;
	struct Chunk* next;
};

struct chunk_neighbours {
	struct Chunk* north;
	struct Chunk* south;
	struct Chunk* east;
	struct Chunk* west;
};

struct tile_data {
	char type[NR_TILES];
};

typedef struct Chunk {
	union chunk_handle;					//12
	uint32_t state;						//16
	uint32_t ent_count;					//20
	struct chunk_node;					//28
	struct chunk_neighbours;			//40
	int component_mask;					//44
	uint16_t* offsets;					//48

	struct tile_data tile_data;
	char data[4];

} Chunk;

typedef struct World {
	int16_t id;
	uint16_t chunkcount;
	uint16_t max_ents;
	uint16_t component_mask;
	Chunk* list;
	Chunk* next;
	uint8_t sizes[MAX_COMPS];
	uint16_t offsets[MAX_COMPS];

} World;

int openlib_Game(lua_State* L);