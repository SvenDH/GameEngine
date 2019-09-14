#include "ecs.h"
#include "graphics.h"

int system_addtype(System* system, Event evt) {
	type_t type = (type_t)evt.data;
	if ((type & system->and_mask) == system->and_mask && !(type & system->not_mask)) {
		Archetype* archetype = archetype_get(system->world, type, 0);
		hashmap_put(&system->archetypes, archetype->type, archetype);
	}
	return 0;
}

int system_removetype(System* system, Event evt) {
	type_t type = (type_t)evt.data;
	if ((type & system->and_mask) == system->and_mask && !(type & system->not_mask)) {
		Archetype* archetype = archetype_get(system->world, type, 0);
		hashmap_remove(&system->archetypes, type);
	}
	return 0;
}

System* system_new(World* wld, Callback cb, type_t and_mask, type_t not_mask, event_t evt) {
	System* system = objectallocator_alloc(&wld->systems, sizeof(System));
	system->and_mask = and_mask;
	system->not_mask = not_mask;
	system->world = wld;
	hashmap_init(&system->archetypes);
	system->data = NULL;
	//Subscribe the system to all events
	EventHandler* handler = eventhandler_instance();
	event_register(handler, system, evt, cb);
	event_register(handler, system, EVT_NEWTYPE, system_addtype);
	event_register(handler, system, EVT_DELTYPE, system_removetype);
	return system;
}

void system_delete(World* wld, System* system) {
	hashmap_free(&system->archetypes);
	objectallocator_free(&wld->systems, system);
}

int draw_system(System* system, Event evt) {
	double dt = (double)evt.nr;
	hashmap_t* type_map = &system->archetypes;
	type_t* type; Archetype* a;
	rid_t tex_id = (rid_t){ .value = 0 };
	texture_t* tex = NULL;
	mat3 transform;

	graphics_t* gfx = graphics_instance();
	resourcemanager_t* rm = resourcemanager_instance();
	hashmap_foreach(type_map, type, a) {
		hashmap_t* chunk_map = &a->chunks;
		entity_t* parent; Chunk* chunk;
		hashmap_foreach(chunk_map, parent, chunk) {
			ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
			ent_sprite* spr = chunk_get_componentarray(chunk, comp_sprite);
			for (int i = 0; i < chunk->ent_count; i++) {
				if (spr[i].texture.value != tex_id.value) {
					tex = resource_get(rm, spr[i].texture);
					tex_id = spr[i].texture;
					if (!tex->loaded) {
						//TODO: figure out how to load texture
					}
				}
				transform_set(transform, t[i].position[0] - spr[i].offset[0], t[i].position[1] - spr[i].offset[1], 0.0f, tex->width, tex->height, 0.0f, 0.0f, 0.0f, 0.0f);
				graphics_draw_quad(gfx,
					tex,
					transform,
					spr[i].index,
					spr[i].color,
					spr[i].alpha);
			}
		}
	}
	return 0;
}

int move_system(System* system, Event evt) {
	double dt = (double)evt.nr;
	hashmap_t* type_map = &system->archetypes;
	type_t* type; Archetype* a;
	hashmap_foreach(type_map, type, a) {
		hashmap_t* chunk_map = &a->chunks;
		entity_t* parent; Chunk* chunk;
		hashmap_foreach(chunk_map, parent, chunk) {
			ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
			ent_physics* phy = chunk_get_componentarray(chunk, comp_physics);
			for (int i = 0; i < chunk->ent_count; i++) {
				t[i].position[0] += phy[i].speed[0] * dt;
				t[i].position[1] += phy[i].speed[1] * dt;

				phy[i].speed[0] += phy[i].acceletation[0] * dt;
				phy[i].speed[1] += phy[i].acceletation[1] * dt;
			}
		}
	}
	return 0;
}
/*
struct debug_info {
	double fps;
	Texture* font;
};

int debug_system(System* system, Event evt) {
	struct debug_info* info;
	char temp[16];
	if (!system->data) {
		info = malloc(sizeof(struct debug_info));
		info->font = texture_get("lucida");
		system->data = info;
	}
	info = system->data;
	switch (evt.type) {
	case EVT_NEWENTITY:
		printf("Created entity: ");
		print_uuid((UID)evt.data);
		fflush(stdout);
		break;
	case EVT_DELENTITY:
		printf("Deleted entity: ");
		print_uuid((UID)evt.data);
		fflush(stdout);
		break;
	case EVT_UPDATE:
		break;
	case EVT_GUI:
		info->fps = 1.0 / (double)evt.data;
		sprintf(temp, "%f", info->fps);
		sprite_draw_text(info->font, temp, 600.0, 4.0, 0, 1.0);
		break;
	}
	return 0;
}
*/