#include "physics.h"
#include "event.h"
#include "utils.h"
#include "math.h"
#include "ecs.h"

void physics_init(physics_t* phs, const char* name) {
	phs->tile_size = TILE_SIZE; //TODO: configurable
	phs->gravity[0] = 0.0f;
	phs->gravity[1] = 10.0f * phs->tile_size * 5;

	object_init(&phs->bodies, name, sizeof(body_t), 64, 16);
	object_init(&phs->nodes, name, sizeof(gridnode_t), 64, 4);
	object_init(&phs->pairs, name, sizeof(collision_t), 64, 4);
	object_init(&phs->cells, name, sizeof(tilecell_t) + TILE_COLS * TILE_ROWS - 2, 64, 64);

	hashmap_init(&phs->objectmap);
	hashmap_init(&phs->bodygrid);
	hashmap_init(&phs->largeobjects);

	memset(&phs->bitmasktable, 0, BITMASKTABLE_SIZE * sizeof(int*));

	bitmask_empty(phs->bitmasks[TILE_EMPTY], phs->tile_size);
	bitmask_solid(phs->bitmasks[TILE_SOLID], phs->tile_size);
	bitmask_slope(phs->bitmasks[TILE_LSLOPE], phs->tile_size, 1, 1);
	bitmask_slope(phs->bitmasks[TILE_RSLOPE], phs->tile_size, 0, 1);

	world_t* world = world_instance();
	system_new(world, cell_update, tiles, tiles | transform, disabled, addcomponent | setcomponent | delcomponent, phs);
	system_new(world, body_update, collider, collider | transform | physics, disabled, addcomponent | setcomponent | delcomponent, phs);
	system_new(world, physics_update, 0, collider | transform | physics, disabled, update, phs);
}

int body_update(system_t* system, event_t evt) {
	ent_transform* t; ent_collider* comp;
	entity_t ent = (entity_t)evt.p1.data;
	physics_t* phs = (physics_t*)system->data;
	switch (evt.type) {
	case on_addcomponent:
		physics_newbody(phs, ent);
		break;
	case on_setcomponent:
		t = entity_getcomponent(system->world, ent, comp_transform);
		comp = entity_getcomponent(system->world, ent, comp_collider);
		physics_movebody(phs, 
			body_get(phs, ent), 
			t->position[0] - comp->offset[0], 
			t->position[1] - comp->offset[1], 
			comp->size[0], comp->size[1]);
		break;
	case on_delcomponent:
		physics_deletebody(phs, ent);
		break;
	}
	return 0;
}

int cell_update(system_t* system, event_t evt) {
	ent_transform* t; ent_tiles* tiles;
	entity_t ent = (entity_t)evt.p1.data;
	physics_t* phs = (physics_t*)system->data;
	switch (evt.type) {
	case on_addcomponent:
		cell_new(phs, ent);
		break;
	case on_setcomponent:
		t = entity_getcomponent(system->world, ent, comp_transform);
		tiles = entity_getcomponent(system->world, ent, comp_tiles);
		cell_set(phs, ent, &tiles->data, 0, TILE_COLS * TILE_ROWS);
		physics_movebody(phs, cell_get(phs, ent),
			t->position[0], t->position[1],
			TILE_SIZE * TILE_COLS, TILE_SIZE * TILE_ROWS);
		break;
	case on_delcomponent:
		cell_delete(phs, ent);
		break;
	}
	return 0;
}

int physics_update(system_t* system, event_t evt) {
	type_t* type; type_data* arch; chunk_data* chunk;
	physics_t* phs = (physics_t*)system->data;
	double dt = evt.p0.nr;
	hashmap_foreach(&system->archetypes, type, arch) {
		archetype_foreach(system->world, arch, chunk) {
			entity_t* id = chunk_get_componentarray(chunk, comp_id);
			ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
			ent_physics* phy = chunk_get_componentarray(chunk, comp_physics);
			ent_collider* comp = chunk_get_componentarray(chunk, comp_collider);
			for (int i = 0; i < chunk->ent_count; i++) {
				body_t* body = body_get(phs, id[i]);
				body_set(body, phy[i].mass, phy[i].friction, phy[i].bounce);
				body_applyforce(body, (vec2) { phs->gravity[0] * phy[i].mass, phs->gravity[1] * phy[i].mass });
				body_applyforce(body, phy[i].force);
			}
		}
	}
	
	//Update physics state
	physics_step(phs, dt);

	//Update bodies
	hashmap_foreach(&system->archetypes, type, arch) {
		archetype_foreach(system->world, arch, chunk) {
			entity_t* id = chunk_get_componentarray(chunk, comp_id);
			ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
			ent_physics* phy = chunk_get_componentarray(chunk, comp_physics);
			ent_collider* comp = chunk_get_componentarray(chunk, comp_collider);
			for (int i = 0; i < chunk->ent_count; i++) {
				body_t* body = body_get(phs, id[i]);
				t[i].position[0] = body->aabb[0] + comp[i].offset[0];
				t[i].position[1] = body->aabb[1] + comp[i].offset[1];
				phy[i].speed[0] = body->speed[0];
				phy[i].speed[1] = body->speed[1];
				body->force[0] = 0.0f;
				body->force[1] = 0.0f;
				//TODO: make set_component function
				event_post(eventhandler_instance(),
					(event_t) {
					.type = on_setcomponent, .p0.data = comp_transform, .p1.data = id[i]
				});
				event_post(eventhandler_instance(),
					(event_t) {
					.type = on_setcomponent, .p0.data = comp_physics, .p1.data = id[i]
				});
			}
		}
	}
	return 0;
}

void physics_step(physics_t* phs, double dt) {
	UID id; body_t* body; body_t* other; collision_t* pair;
	//Calculate new speed from force
	hashmap_foreach(&phs->objectmap, id, body)
		physics_integrateforces(phs, body, dt / 2.0f);

	//Update pair parameters
	float grav = vec2_len((vec2) { dt* phs->gravity[0], dt* phs->gravity[1] });
	hashmap_foreach(&phs->objectmap, id, body)
		hashmap_foreach(&body->pairs, other, pair)
			if (pair->colliding)
				collision_update(pair, grav);

	//Calculate impulses for number of iterations
	for (int j = 0; j < PHYSICS_ITERATIONS; j++)
		hashmap_foreach(&phs->objectmap, id, body)
			hashmap_foreach(&body->pairs, other, pair)
				if (pair->colliding)
					collision_applyimpulse(pair);

	//Move bodies and recalculate speed
	hashmap_foreach(&phs->objectmap, id, body)
		physics_integratespeed(phs, body, dt);

	//Correct positions when penetrating
	hashmap_foreach(&phs->objectmap, id, body)
		hashmap_foreach(&body->pairs, other, pair)
			if (pair->colliding)
				collision_correction(phs, pair);
}

void physics_colliderinit(physics_t* phs, collider_t* body, UID id, int type, int is_static) {
	assert(!hashmap_get(&phs->objectmap, id));
	body->id = id;
	body->type = type;
	body->layer = 0;
	body->mask = 1;
	body->friction = 0.1f;
	body->inv_mass = 0.0f;
	body->bounce = 0.0f;
	body->_static = is_static;
	memcpy(body->aabb, &NULL_RECT, sizeof(rect2));
	hashmap_init(&body->pairs);
	hashmap_put(&phs->objectmap, id, body);
}

void physics_colliderdelete(physics_t* phs, collider_t* body) {
	if (memcmp(body->aabb, &NULL_RECT, sizeof(rect2)) != 0)
		_physics_bodyremove(phs, body, body->aabb);
	hashmap_free(&body->pairs);
	hashmap_remove(&phs->objectmap, id);
}

void physics_integrateforces(physics_t* phs, body_t* body, float dt) {
	if (body->inv_mass == 0.0f)
		return;

	body->speed[0] += (body->force[0] * body->inv_mass) * dt;
	body->speed[1] += (body->force[1] * body->inv_mass) * dt;
}

void physics_integratespeed(physics_t* phs, body_t* body, float dt) {
	if (body->inv_mass == 0.0f)
		return;

	float dx = body->speed[0] * dt;
	float dy = body->speed[1] * dt;
	physics_movebody(phs, body, body->aabb[0] + dx, body->aabb[1] + dy, body->aabb[2] - body->aabb[0], body->aabb[3] - body->aabb[1]);
	physics_integrateforces(phs, body, dt / 2.0f);
}

void physics_movebody(physics_t* phs, collider_t* body, float x, float y, float width, float height) {
	rect2 aabb = { x, y, x + width, y + height };
	if (memcmp(aabb, body->aabb, sizeof(rect2)) == 0)
		return;
	if (memcmp(aabb, &NULL_RECT, sizeof(rect2)) != 0)
		_physics_bodyinsert(phs, body, aabb);
	if (memcmp(body->aabb, &NULL_RECT, sizeof(rect2)) != 0)
		_physics_bodyremove(phs, body, body->aabb);
	memcpy(body->aabb, aabb, sizeof(rect2));

	collider_t* other; collision_t* pair; int collision;
	hashmap_foreach(&body->pairs, other, pair) {
		if (!(1 << other->layer & body->mask)) collision = 0;
		else collision = collision_solve(phs, pair);

		if ((collision > 0) != (pair->colliding > 0))
			if (collision) physics_onstartcollide(phs, body, other);
			else physics_onstopcollide(phs, body, other);
		
		pair->colliding = collision;
	}
}

void physics_onstartcollide(physics_t* phs, body_t* body, body_t* other) {
	event_post(eventhandler_instance(), (event_t) { .type = on_startcollision, .p0.data = body->id, .p1.data = other->id });
}

void physics_onstopcollide(physics_t* phs, body_t* body, body_t* other) {
	event_post(eventhandler_instance(), (event_t) { .type = on_endcollision, .p0.data = body->id, .p1.data = other->id });
}

void _physics_pair(physics_t* phs, collider_t* body, collider_t* other) {
	collision_t* pair = hashmap_get(&body->pairs, other);
	if (!pair) {
		pair = object_alloc(&phs->pairs, sizeof(collision_t));
		collision_init(pair, body, other);
		hashmap_put(&body->pairs, other, pair);
		hashmap_put(&other->pairs, body, pair);
	}
	pair->rc++;
}

void _physics_unpair(physics_t* phs, collider_t* body, collider_t* other) {
	collision_t* pair = hashmap_get(&body->pairs, other);
	if (pair) {
		pair->rc--;
		if (pair->rc == 0) {
			if (pair->colliding)
				physics_onstopcollide(phs, body, other);

			object_free(&phs->pairs, pair);
			hashmap_remove(&body->pairs, other);
			hashmap_remove(&other->pairs, body);
		}
	}
	else printf("Error: pair does not exist");
}

void _physics_bodyinsert(physics_t* phs, collider_t* body, rect2 aabb) {
	const float tile_size = phs->tile_size;
	int rc; UID id; collider_t* other;
	vec2 sz = { (aabb[0] - aabb[2]) / tile_size * LARGE_ELEMENT_FI, (aabb[1] - aabb[3]) / tile_size * LARGE_ELEMENT_FI };
	if (sz[0] * sz[1] > BIGCOLLIDERSIZE) {
		//large object, do not use grid, must check against all elements
		hashmap_foreach(&phs->objectmap, id, other) {
			if (other == body)
				continue; // do not pair against itself
			if (other->_static && body->_static)
				continue;
			_physics_pair(phs, other, body);
		}
		rc = hashmap_get(&phs->largeobjects, body);
		hashmap_put(&phs->largeobjects, body, ++rc);
		return;
	}
	//Body collision
	short tile_x0 = floor(aabb[0] / tile_size);
	short tile_y0 = floor(aabb[1] / tile_size);
	short tile_x1 = floor(aabb[2] / tile_size);
	short tile_y1 = floor(aabb[3] / tile_size);
	for (int i = tile_x0; i <= tile_x1; i++) {
		for (int j = tile_y0; j <= tile_y1; j++) {
			uint32_t c = mortonencode(i, j);
			//Insert body into hashgrid
			gridnode_t* node = hashmap_get(&phs->bodygrid, c);
			int inserted = 0;
			if (!node) {
				node = object_alloc(&phs->nodes, sizeof(gridnode_t));
				hashmap_init(&node->objects);
				hashmap_put(&phs->bodygrid, c, node);
			}
			else inserted = hashmap_get(&node->objects, body);
			if (inserted == 0 && !hashmap_isempty(&node->objects)) {
				hashmap_foreach(&node->objects, other, rc)
					_physics_pair(phs, body, other);
			}
			hashmap_put(&node->objects, body, ++inserted);
		}
	}
	//Pair separatedly with large colliders
	hashmap_foreach(&phs->largeobjects, other, rc) {
		if (other == body)
			continue;
		if (other->_static && body->_static)
			continue;
		_physics_pair(phs, body, other);
	}
}

void _physics_bodyremove(physics_t* phs, collider_t* body, rect2 aabb) {
	const float tile_size = phs->tile_size;
	int rc; UID id; collider_t* other;
	vec2 sz = { (aabb[0] - aabb[2]) / tile_size * LARGE_ELEMENT_FI, (aabb[1] - aabb[3]) / tile_size * LARGE_ELEMENT_FI };
	if (sz[0] * sz[1] > BIGCOLLIDERSIZE) {
		//Large objects do not use grid, must check against all elements
		hashmap_foreach(&body->pairs, other, rc)
			_physics_unpair(phs, other, body);

		rc = hashmap_get(&phs->largeobjects, body);
		rc--;
		if (rc == 0) hashmap_remove(&phs->largeobjects, body);
		else hashmap_put(&phs->largeobjects, body, rc);
		return;
	}
	short tile_x0 = floor(aabb[0] / tile_size);
	short tile_y0 = floor(aabb[1] / tile_size);
	short tile_x1 = floor(aabb[2] / tile_size);
	short tile_y1 = floor(aabb[3] / tile_size);
	for (int i = tile_x0; i <= tile_x1; i++) {
		for (int j = tile_y0; j <= tile_y1; j++) {
			uint32_t c = mortonencode(i, j);
			//Remove body from hashgrid
			gridnode_t* node = hashmap_get(&phs->bodygrid, c);
			rc = hashmap_get(&node->objects, body);
			if (rc == 1) {
				hashmap_remove(&node->objects, body);
				if (hashmap_isempty(&node->objects)) {
					hashmap_free(&node->objects);
					hashmap_remove(&phs->bodygrid, c);
					object_free(&phs->nodes, node);
				}
				else {
					hashmap_foreach(&node->objects, other, rc) {
						if (other == body)
							continue;
						_physics_unpair(phs, body, other);
					}
				}
			}
			else hashmap_put(&node->objects, body, --rc);
		}
	}
	hashmap_foreach(&phs->largeobjects, other, rc) {
		if (other == body)
			continue;
		if (other->_static && body->_static)
			continue;
		_physics_unpair(phs, body, other);
	}
}

//Physics.set_bitmaps{ 0, 0, 1, ... }
static int w_physics_setbitmaps(lua_State* L) {
	physics_t* phs = physics_instance();
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		int i = (int)lua_tointeger(L, -1) - 1;
		int v = (int)lua_tointeger(L, -2);
		phs->bitmasktable[i] = v;
		lua_pop(L, 2);
	}
	return 0;
}

int openlib_Physics(lua_State* L) {
	static luaL_Reg phs_lib[] = {
		{"set_bitmaps", w_physics_setbitmaps},
		{NULL, NULL}
	};
	create_lua_lib(L, Physics_mt, phs_lib);
	return 0;
}
