#pragma once
#include <stdint.h>

#define CW 32
#define CH 32
#define RW 8
#define RH 8
#define TW 16
#define TH 16
#define ENT_COUNT 256
#define ENT_SIZE 16
#define CHUNK_COUNT 32
#define CHUNK_SIZE 4 + 4 + 4 + CW * CH + ENT_COUNT * ENT_SIZE
#define CHUNK_POOL_SIZE 64

typedef enum {
	ENT_NONE = 0,
} entity_t;

typedef enum {
	ENT_STATE_DEAD = 0,
	ENT_STATE_IDLE,
	ENT_STATE_RUN,
	ENT_STATE_ATTACK,
	ENT_STATE_FALL,
	ENT_STATE_JUMP
} entity_state;

typedef enum {
	CHUNK_NONE = 0,
	CHUNK_EXIST = 1,
	CHUNK_LOADING = 2
} chunk_t;

enum dir {
	NONE = 0,  //0000
	LEFT = 1,  //0001
	TOP = 2,   //0010
	RIGHT = 4, //0100
	BOTTOM = 8 //1000
};

typedef struct Room {
	int con;
	int var;
	int w, h;
	char tiles[RW * RH];
} Room;

typedef struct RoomData {
	Room *rooms;
} RoomData;

void room_add(RoomData *roomdata, unsigned char *data);

void level_init();
void level_render();
void level_update(double dt);
void level_clear();

typedef struct EntityData_ {
	union {
		struct {
			uint8_t type; //entity components
			uint8_t state;
			uint8_t variant;
			uint8_t data[3];
			int8_t hsp;
			int8_t vsp;

			int16_t x;
			int16_t y;

			uint32_t time;
		};
		uint8_t bytes[ENT_SIZE];
	};
} EntityData;
typedef EntityData *Entity;

void entity_init(Entity entity, int x, int y, entity_t type);
void entity_render(Entity entity);
void entity_update(Entity entity, double dt);
void entity_clear(Entity entity);

typedef struct ChunkData_ {
	uint8_t type; //chunk type (0 is none)
	uint8_t index;
	union {
		struct {
			uint32_t x; //chunk coords
			uint32_t y;
			uint32_t nr_ents;

			uint8_t tiles[CW*CH]; //chunk tiles
			EntityData entities[ENT_COUNT];
		};
		uint8_t bytes[CHUNK_SIZE];
	};
} ChunkData, ChunkPool[CHUNK_POOL_SIZE];
typedef ChunkData *Chunk;

void chunk_init(Chunk chunk, int x, int y);
void chunk_render(Chunk chunk);
void chunk_update(Chunk chunk, double dt);
void chunk_clear(Chunk chunk);