#include "physics.h"
#include "math.h"
#include "bitmask.h"


int _update_bodyvsbody(Collision* pair, rect2 A, rect2 B) {
	vec2 d = { B[0] - A[0], B[1] - A[1] };
	float a_extent = (A[2] - A[0]) / 2;
	float b_extent = (B[2] - B[0]) / 2;
	float x_overlap = a_extent + b_extent - fabs(d[0]);
	//Seperate axis test
	if (x_overlap > 0) {
		a_extent = (A[3] - A[1]) / 2;
		b_extent = (B[3] - B[1]) / 2;
		float y_overlap = a_extent + b_extent - fabs(d[1]);
		if (y_overlap > 0) {
			if (x_overlap < y_overlap) {
				pair->normal[0] = (d[0] < 0) ? -1.0f : 1.0f;
				pair->normal[1] = 0.0f;
				pair->penetration = x_overlap;
			}
			else {
				pair->normal[0] = 0.0f;
				pair->normal[1] = (d[1] < 0) ? -1.0f : 1.0f;
				pair->penetration = y_overlap;
			}
			return 1;
		}
	}
	return 0;
}

int _update_bodyvstile(Collision* pair, rect2 A, rect2 B, byte* tiles, struct tile_info* tileinfo) {
	int tile_size = tileinfo->size;
	int xr = 0, xl = 0, yd = 0, yu = 0;
	uint32_t width = (B[2] - B[0]) / tile_size;
	uint32_t height = (B[3] - B[1]) / tile_size;
	rect2 tile_range = { floor((A[0] - B[0]) / tile_size), floor((A[1] - B[1]) / tile_size), floor((A[2] - B[0]) / tile_size), floor((A[3] - B[1]) / tile_size) };
	for (int j = max(tile_range[1], 0); j <= min(tile_range[3], height - 1); j++) {
		for (int i = max(tile_range[0], 0); i <= min(tile_range[2], width - 1); i++) {
			int tile_idx = tileinfo->tilemap[tiles[i + j * width]];
			if (tile_idx != TILE_EMPTY) {
				rect2 taabb = { 
					A[0] - B[0] - i * tile_size,  
					A[1] - B[1] - j * tile_size, 
					A[2] - B[0] - i * tile_size,  
					A[3] - B[1] - j * tile_size};
				int or , ol, od, ou;
				if (tile_idx == TILE_SOLID) {
					or = tile_size - taabb[0] + 1;
					ol = taabb[2] + 1;
					od = tile_size - taabb[1] + 1;
					ou = taabb[3] + 1;
				}
				else { //Slope
					or = bitmask_offset(tileinfo->bitmasks[tile_idx], tile_size, taabb, 1.0f, 0);
					ol = bitmask_offset(tileinfo->bitmasks[tile_idx], tile_size, taabb, -1.0f, 0);
					od = bitmask_offset(tileinfo->bitmasks[tile_idx], tile_size, taabb, 0, 1.0f);
					ou = bitmask_offset(tileinfo->bitmasks[tile_idx], tile_size, taabb, 0, -1.0f);
				}
				if (or < ol) 
					xr = max(or , xr);
				else
					xl = max(ol, xl);
				if (od < ou) 
					yd = max(od, yd);
				else 
					yu = max(ou, yu);
			}
		}
	}
	if (xr || xl || yd || yu) {
		if ((xr != 0) ^ (xl != 0) && (yd != 0) ^ (yu != 0)) { //On point
			if (max(xr,xl) < max(yd, yu)) {
				pair->normal[0] = (xr > xl) ? -xr + residual(A[0]) : xl - (1 - residual(A[2]));
				pair->normal[1] = 0.0f;
			}
			else {
				pair->normal[0] = 0.0f;
				pair->normal[1] = (yd > yu) ? -yd + residual(A[1]) : yu - (1 - residual(A[3]));
			}
		}
		else if (xr && xl && yd && yu) { //In corner
			pair->normal[0] = (xr < xl) ? -xr + residual(A[0]) : xl - (1 - residual(A[2]));
			pair->normal[1] = (yd < yu) ? -yd + residual(A[1]) : yu - (1 - residual(A[3]));
		}
		else if (xr && xl) { //On ground
			pair->normal[0] = 0.0f;     
			pair->normal[1] = (yd > yu) ? -yd + residual(A[1]) : yu - (1 - residual(A[3]));
		}
		else if(yd && yu) { //On wall
			pair->normal[0] = (xr > xl) ? -xr + residual(A[0]) : xl - (1 - residual(A[2]));
			pair->normal[1] = 0.0f;
		}
		pair->penetration = vec2_len(pair->normal);
		vec2_normalize(pair->normal);

		return 1;
	}
	return 0;
}

void _bodytile_solve(Collision* pair, Body* A) {
	float inv_mass = A->inv_mass;
	if (inv_mass <= EPSILON) {
		A->speed[0] = 0.0f; A->speed[1] = 0.0f;
		return;
	}
	vec2 rv = { -A->speed[0], -A->speed[1] };
	float contactv = vec2_dot(rv, pair->normal);
	if (contactv > 0)
		return;

	float j = -(1.0f + pair->bounce) * contactv / inv_mass;
	vec2 impulse = { -pair->normal[0] * j, -pair->normal[1] * j };
	body_applyimpulse(A, impulse);

	float rvn = vec2_dot(rv, pair->normal);
	vec2 t = { rv[0] - (pair->normal[0] * rvn), rv[1] - (pair->normal[1] * rvn) };
	vec2_normalize(t);
	float jt = -vec2_dot(rv, t) / inv_mass;
	if (fabs(jt) <= EPSILON)
		return;
	//TODO: fix friction
	vec2 tan_impulse = { -t[0], -t[1] };
	if (fabs(jt) < j * pair->friction) {
		tan_impulse[0] *= jt;
		tan_impulse[1] *= jt;
	}
	else {
		tan_impulse[0] *= -j * pair->friction;
		tan_impulse[1] *= -j * pair->friction;
	}
	body_applyimpulse(A, tan_impulse);
}

void _bodypair_solve(Collision* pair, Body* A, Body* B) {
	if (A->inv_mass + B->inv_mass <= EPSILON) {
		A->speed[0] = 0.0f; A->speed[1] = 0.0f;
		B->speed[0] = 0.0f; B->speed[1] = 0.0f;
		return;
	}
	vec2 rv = { B->speed[0] - A->speed[0], B->speed[1] - A->speed[1] };
	float contactv = vec2_dot(rv, pair->normal);
	if (contactv > 0)
		return;

	float inv_mass = A->inv_mass + B->inv_mass;
	float j = -(1.0f + pair->bounce) * contactv / inv_mass;
	vec2 impulse = { -pair->normal[0] * j, -pair->normal[1] * j };
	body_applyimpulse(A, impulse);
	body_applyimpulse(B, (vec2) { -impulse[0], -impulse[1] });

	float rvn = vec2_dot(rv, pair->normal);
	vec2 t = { rv[0] - (pair->normal[0] * rvn), rv[1] - (pair->normal[1] * rvn) };
	vec2_normalize(t);
	float jt = -vec2_dot(rv, t) / inv_mass;
	if (fabs(jt) <= EPSILON)
		return;
	//TODO: fix friction
	vec2 tan_impulse = { -t[0], -t[1] };
	if (fabs(jt) < j * pair->friction) {
		tan_impulse[0] *= jt;
		tan_impulse[1] *= jt;
	}
	else {
		tan_impulse[0] *= -j * pair->friction;
		tan_impulse[1] *= -j * pair->friction;
	}
	body_applyimpulse(A, tan_impulse);
	body_applyimpulse(B, (vec2) { -tan_impulse[0], -tan_impulse[1] });
}

SYSTEM_ENTRY(contraint_update) {
	double dt = task->delta_time;
	physics_t* phs = task->data;
	//TODO: check collision for space
	world_t* wld = task->wld;
	COMPARRAY_GET(Collision, pair, task);
	for (int i = task->start; i < task->end; i++) {
		entity_t A = pair[i].A;
		entity_t B = pair[i].B;
		ENTITY_GETCOMP(wld, Collider, cA, A);
		ENTITY_GETCOMP(wld, Collider, cB, B);
		ENTITY_GETCOMP(wld, Transform, tA, A);
		ENTITY_GETCOMP(wld, Transform, tB, B);
		ENTITY_GETCOMP(wld, Body, bA, A);
		ENTITY_GETCOMP(wld, Tiles, tileB, B);
		rect2 aabbA, aabbB;
		collider_getaabb(cA, tA, aabbA);
		collider_getaabb(cB, tB, aabbB);
		vec2 rv = { -bA->speed[0], -bA->speed[1] };
		if (!tileB) {
			_update_bodyvsbody(pair, aabbA, aabbA);

			ENTITY_GETCOMP(wld, Body, bB, pair[i].B);
			rv[0] += bB->speed[0];
			rv[1] += bB->speed[1];

			//TODO: fix this without gravity:
			float grav_len = vec2_len((vec2) { dt* phs->gravity[0], dt* phs->gravity[1] });
			if (vec2_len(rv) < grav_len + EPSILON)
				pair->bounce = 0.0;
			else
				pair->bounce = min(bA->bounce, bB->bounce);

			pair->friction = sqrt((double)(bA->friction * bB->friction));
		}
		else {
			_update_bodyvstile(pair, aabbA, aabbB, tileB, &phs->default_space.tiles);
			//TODO: get tile friction and bounce
			pair->bounce = 0.0f;
			pair->friction = sqrt((double)(bA->friction * 0.1));
		}
	}
}

SYSTEM_ENTRY(contraint_solve) {
	double dt = task->delta_time;
	space_t* spc = task->data;
	world_t* wld = task->wld;
	COMPARRAY_GET(Collision, collision, task);
	for (int p = 0; p < PHYSICS_ITERATIONS; p++) {
		for (int i = task->start; i < task->end; i++) {
			ENTITY_GETCOMP(wld, Body, A, collision[i].A);
			ENTITY_GETCOMP(wld, Body, B, collision[i].B);
			if (A && B)
				_bodypair_solve(&collision[i], A, B);
			else if (A)
				_bodytile_solve(&collision[i], A);
		}
	}
}

SYSTEM_ENTRY(contraint_correction) {
	world_t* wld = task->wld;
	COMPARRAY_GET(Collision, collision);
	for (int i = task->start; i < task->end; i++) {
		ENTITY_GETCOMP(wld, Body, A, collision[i].A);
		ENTITY_GETCOMP(wld, Body, B, collision[i].B);
		float A_inv_mass = A ? A->inv_mass : 0.0f;
		float B_inv_mass = B ? B->inv_mass : 0.0f;
		//TODO: check collider type
		float penetration_depth = max(collision[i].penetration - PENETRATION_SLOP, 0.0f) / (A_inv_mass + B_inv_mass) * PENETRATION_PERCENT;
		vec2 correction = { penetration_depth * collision[i].normal[0], penetration_depth * collision[i].normal[1] };
		if (A) {
			ENTITY_GETCOMP(wld, Transform, tA, collision[i].A);
			tA->position[0] -= A_inv_mass * correction[0];
			tA->position[1] -= A_inv_mass * correction[1];
		}
		if (B) {
			ENTITY_GETCOMP(wld, Transform, tB, collision[i].B);
			tB->position[0] += B_inv_mass * correction[0];
			tB->position[0] += B_inv_mass * correction[1];
		}
	}
}


