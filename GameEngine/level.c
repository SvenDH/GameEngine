#include "level.h"
#include "graphics.h"

#include <stdlib.h>

int seed;
ChunkPool chunks;

Texture *tileset;

void level_init() {
	seed = rand();

	for (int i = 0; i < CHUNK_POOL_SIZE; i++)
		chunks[i].index = i;

	tileset = NULL;//(Texture*)resources_get("castle");

	chunk_init(&chunks[0], 0, 0);
}

void level_render() {
	int i, j;
	for (i = 0; i < CHUNK_POOL_SIZE; i++)
		if (chunks[i].type)
			chunk_render(&chunks[i]);
	for (i = 0; i < CHUNK_POOL_SIZE; i++)
		if (chunks[i].type)
			for (j = 0; j < chunks[i].nr_ents; j++)
				entity_render(&(chunks[i].entities[j]));
}

void level_update(double dt) {
	int i;
	for (i = 0; i < CHUNK_POOL_SIZE; i++)
		if (chunks[i].type)
			chunk_update(&chunks[i], dt);
}

void level_clear() {
	for (int i = 0; i < CHUNK_POOL_SIZE; i++)
		if (chunks[i].type)
			chunk_clear(&chunks[i]);
}

void room_add(RoomData *roomdata, unsigned char *data) {
	roomdata->rooms = malloc(16 * sizeof(RoomData));
}

void chunk_init(Chunk chunk, int x, int y) {
	chunk->type = CHUNK_EXIST | CHUNK_LOADING;
	chunk->x = x;
	chunk->y = y;
	chunk->nr_ents = 0;
	//Generate tiles
	for (int i = 0; i < CW*CH; i++) {
		chunk->tiles[i] = rand();
	}
}

void chunk_render(Chunk chunk) {
	//render_tiles(tileset, tilemap, chunk->index, chunk->x, chunk->y, 0, CW*TW, CH*TH);
}

void chunk_update(Chunk chunk, double dt) {
	for (int i = 0; i < chunk->nr_ents; i++)
		entity_update(&chunk->entities[i], dt);
}

void chunk_clear(Chunk chunk) {
	chunk->type = CHUNK_NONE;
}

void entity_init(Entity entity, int x, int y, entity_t type) {
	entity->type = type;
	entity->x = x;
	entity->y = y;
	entity->hsp = 0;
	entity->vsp = 0;
	entity->state = ENT_STATE_IDLE;
}

void entity_render(Entity entity) {

}

void entity_update(Entity entity, double dt) {

}

void entity_clear(Entity entity) {
	entity->type = ENT_NONE;
}