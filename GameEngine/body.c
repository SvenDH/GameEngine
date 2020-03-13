#include "physics.h"

body_t* physics_newbody(physics_t* phs, UID id) {
	body_t* body = object_alloc(&phs->bodies, sizeof(body_t));
	physics_colliderinit(phs, body, id, BODY_COLL, 0);
	memset(body->speed, 0, sizeof(vec2));
	memset(body->force, 0, sizeof(vec2));
	return body;
}

void physics_deletebody(physics_t* phs, UID id) {
	body_t* body = body_get(phs, id);
	physics_colliderdelete(phs, body);
	object_free(&phs->bodies, body);
}

body_t* body_get(physics_t* phs, UID id) {
	return hashmap_get(&phs->objectmap, id);
}

void body_set(body_t* body, float mass, float friction, float bounce) {
	if (mass) body->inv_mass = 1.0f / mass;
	else body->inv_mass = 0.0f;
	body->friction = friction;
	body->bounce = bounce;
}

void body_applyforce(body_t* body, vec2 force) {
	body->force[0] += force[0];
	body->force[1] += force[1];
}

void body_applyimpulse(body_t* body, vec2 impulse) {
	body->speed[0] += impulse[0] * body->inv_mass;
	body->speed[1] += impulse[1] * body->inv_mass;
}