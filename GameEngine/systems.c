#include "ecs.h"
#include "graphics.h"
#include "physics.h"


system_t* system_new(world_t* wld, evt_callback_t cb, type_t react_mask, type_t and_mask, type_t not_mask, int evt, void* data) {
	system_t* system = object_alloc(&wld->systems, sizeof(system_t));
	list_add(&wld->system_list, system);
	system->mask.react = react_mask;
	system->mask.and = and_mask;
	system->mask.not = not_mask;
	system->world = wld;
	system->cb = cb;
	system->evt = evt;
	system->data = data;
	hashmap_init(&system->archetypes);

	return system;
}

void system_delete(world_t* wld, system_t* system) {
	hashmap_free(&system->archetypes);
	list_remove(&wld->system_list, system);
	object_free(&wld->systems, system);
}

int system_update(world_t* wld, event_t evt) {
	system_t* system;
	list_foreach(&wld->system_list, system)
		if ((1 << evt.type) == system->evt)
			system->cb(system, evt);

	return 0;
}

int system_listener(world_t* wld, event_t evt) {
	type_t type = (type_t)evt.p0.data;
	system_t* system;
	type_t ent_type;
	int i;
	switch (evt.type) {
	case on_addtype:
		list_foreach(&wld->system_list, system)
			if ((type & system->mask.and) == system->mask. and &&!(type & system->mask.not)) {
				type_data* archetype = archetype_get(wld, type, 0);
				if (archetype) hashmap_put(&system->archetypes, archetype->type, archetype);
			}
		break;
	case on_deltype:
		list_foreach(&wld->system_list, system)
			if ((type & system->mask.and) == system->mask. and &&!(type & system->mask.not))
				hashmap_remove(&system->archetypes, type);
		break;
	case on_addcomponent:
	case on_setcomponent:
	case on_delcomponent:
		ent_type = entity_gettype(wld, (entity_t)evt.p1.data, NULL);
		list_foreach(&wld->system_list, system)
			if (((1 << evt.type) & system->evt)
				&& ((1 << type) & system->mask.react)
				&& (ent_type & system->mask.and) == system->mask. and
				&&!(ent_type & system->mask.not)) {
				system->cb(system, evt);
			}
		break;
	}
	return 0;
}

int debug_system(system_t* system, event_t evt) {
	type_t* type; type_data* arch; chunk_data* chunk;
	physics_t* phs = physics_instance();
	graphics_t* gfx = graphics_instance();
	mat3 transform;
	double dt = evt.p0.nr;
	hashmap_foreach(&system->archetypes, type, arch) {
		archetype_foreach(system->world, arch, chunk) {
			entity_t* id = chunk_get_componentarray(chunk, comp_id);
			ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
			ent_physics* phy = chunk_get_componentarray(chunk, comp_physics);
			ent_collider* comp = chunk_get_componentarray(chunk, comp_collider);
			for (int i = 0; i < chunk->ent_count; i++) {
				body_t* body = body_get(phs, id[i]);
				int c = RED;
				body_t* o;
				collision_t* p;
				if (!hashmap_isempty(&body->pairs))
					hashmap_foreach(&body->pairs, o, p)
						if (p->colliding) {
							c = GREEN;
							break;
						}
				transform_set(
					transform,
					t[i].position[0] - comp[i].offset[0],
					t[i].position[1] - comp[i].offset[1],
					0.0f,
					comp[i].size[0], comp[i].size[1],
					0.0f, 0.0f, 0.0f, 0.0f);
				graphics_draw_quad(gfx,
					gfx->default_texture,
					transform,
					0,
					c,
					1.0);
			}
		}
	}
	return 0;
}