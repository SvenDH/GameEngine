#include "systems.h"
#include "sprite.h"

static System systems[MAX_SYSTEMS];

void system_addworld(System* system, event_t evt, char* data, size_t len) {
	int world_id = (int)*data;
	World* wld = world_get(world_id);
	if (wld->component_mask & system->mask )
		system->worlds[system->nr_worlds++] = world_id;
}

System* system_init(event_t evt, event_cb cb, int component_mask) {
	static int id = 1;
	assert(id < MAX_SYSTEMS + 1);
	System* system = &systems[id];
	system->id = id++;
	system->mask = component_mask | COMP_MASK(id);
	system->nr_worlds = 0;
	event_register(evt, cb, system);
	event_register(EVT_NEWWORLD, system_addworld, system);
	return system;
}

void system_iter(System* system, chunk_iter_cb callback) {
	World* wld;
	Chunk* chk;
	int nr_chunks, i, j;
	for (i = 0; i < system->nr_worlds; ++i) {
		wld = world_get(system->worlds[i]);
		nr_chunks = wld->chunkcount;
		for (j = 0; nr_chunks; j++) {
			chk = wld->chunks[j];
			if (chk) {
				--nr_chunks;
				callback(chk);
			}
		}
	}
}

void debug_system(System* system, event_t evt, char* data, size_t len) {
	switch (evt) {
	case EVT_NEWENTITY:
		printf("Created entity: ");
		print_uuid(*((UID*)data));
		fflush(stdout);
		break;
	case EVT_DELENTITY:
		printf("Deleted entity: ");
		print_uuid(*((UID*)data));
		fflush(stdout);
		break;
	}
}

void draw_ent(Chunk* chunk) {
	ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
	ent_sprite* spr = chunk_get_componentarray(chunk, comp_sprite);
	for (int i = 0; i < chunk->ent_count; i++) {
		sprite_draw_quad(
			spr[i].texture, 
			spr[i].index, 
			t[i].position[0] + spr[i].offset[0],
			t[i].position[1] + spr[i].offset[1],
			spr[i].size[0],
			spr[i].size[1],
			0, 0, 1, 1,
			spr[i].color,
			spr[i].alpha);
	}
}

void draw_system(System* system, event_t evt, char* data, size_t len) {
	system_iter(system, draw_ent);
}