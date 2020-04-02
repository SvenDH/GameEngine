#include "physics.h"

SYSTEM_ENTRY(body_new) {
	COMPARRAY_GET(Body, body);
	for (int i = task->start; i < task->end; i++) {
		body[i].mass = 0.0f;
		body[i].inv_mass = 0.0f;
		body[i].force[0] = 0.0f;
		body[i].force[1] = 0.0f;
		body[i].speed[0] = 0.0f;
		body[i].speed[1] = 0.0f;
		body[i].bounce = 0.0f;
		body[i].friction = 0.1f;
	}
}

SYSTEM_ENTRY(body_set) {
	COMPARRAY_GET(Body, body);
	for (int i = task->start; i < task->end; i++) {
		if (body[i].mass)
			body[i].inv_mass = 1.0f / body[i].mass;
		else
			body[i].inv_mass = 0.0f;
	}
}

SYSTEM_ENTRY(body_integrateforces) {
	double dt = task->delta_time;
	physics_t* phs = task->data;
	COMPARRAY_GET(EntityId, id);
	COMPARRAY_GET(Body, body);
	for (int i = task->start; i < task->end; i++) {
		if (body[i].inv_mass == 0.0f)
			continue;

		body[i].speed[0] += (body[i].force[0] * body[i].inv_mass + phs->gravity[0]) * dt;
		body[i].speed[1] += (body[i].force[1] * body[i].inv_mass + phs->gravity[1]) * dt;
		//TODO: extend aabb for motion
		//space_movebody(phs, spc, body, &body->aabb);
		event_post((event_t) {.type = on_setcomponent, .p0.data = id[i].id, .p1.data = COMP_TYPE(Body)});
	}
}

SYSTEM_ENTRY(body_integratespeed) {
	double dt = task->delta_time;
	COMPARRAY_GET(EntityId, id);
	COMPARRAY_GET(Transform, t);
	COMPARRAY_GET(Body, body);
	for (int i = task->start; i < task->end; i++) {
		if (body[i].inv_mass == 0.0f)
			continue;

		t[i].position[0] += body[i].speed[0] * dt;
		t[i].position[1] += body[i].speed[1] * dt;

		event_post((event_t) {.type = on_setcomponent, .p0.data = id[i].id, .p1.data = COMP_TYPE(Transform)});
	
		body[i].force[0] = 0.0f;
		body[i].force[1] = 0.0f;
	}
}

void body_applyforce(Body* body, vec2 force) {
	body->force[0] += force[0];
	body->force[1] += force[1];
}

void body_applyimpulse(Body* body, vec2 impulse) {
	body->speed[0] += impulse[0] * body->inv_mass;
	body->speed[1] += impulse[1] * body->inv_mass;
}
