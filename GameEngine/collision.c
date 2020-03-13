#include "physics.h"
#include "math.h"

void collision_init(collision_t* pair, collider_t* A, collider_t* B) {
	pair->colliding = 0;
	pair->rc = 0;
	pair->A = A;
	pair->B = B;
	pair->normal[0] = 0.0f;
	pair->normal[1] = 0.0f;
	pair->penetration = 0.0f;
	pair->friction = 0.0f;
	pair->bounce = 0.0f;
}

void collision_update(collision_t* pair, float grav_len) {
	//TODO: Generalize gravity out
	collider_t* A = pair->A;
	collider_t* B = pair->B;
	vec2 rv;
	collider_relativespeed(A, B, rv);
	if (vec2_len(rv) < grav_len + EPSILON)
		pair->bounce = 0.0;
	else
		pair->bounce = min(A->bounce, B->bounce);

	pair->friction = sqrt((double)(A->friction * B->friction));
}

int collision_bodyvsbody(physics_t* phs, collision_t* pair, body_t* A, body_t* B) {
	vec2 d = { B->aabb[0] - A->aabb[0], B->aabb[1] - A->aabb[1] };
	float a_extent = (A->aabb[2] - A->aabb[0]) / 2;
	float b_extent = (B->aabb[2] - B->aabb[0]) / 2;
	float x_overlap = a_extent + b_extent - fabs(d[0]);
	//Seperate axis test
	if (x_overlap > 0) {
		a_extent = (A->aabb[3] - A->aabb[1]) / 2;
		b_extent = (B->aabb[3] - B->aabb[1]) / 2;
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

int collision_bodyvstile(physics_t* phs, collision_t* pair, body_t* A, tilecell_t* B) {
	int xr = 0, xl = 0, yd = 0, yu = 0;
	int width = (B->aabb[2] - B->aabb[0]) / phs->tile_size;
	int height = (B->aabb[3] - B->aabb[1]) / phs->tile_size;
	short tile_x0 = max(floor((A->aabb[0] - B->aabb[0]) / phs->tile_size), 0);
	short tile_y0 = max(floor((A->aabb[1] - B->aabb[1]) / phs->tile_size), 0);
	short tile_x1 = min(floor((A->aabb[2] - B->aabb[0]) / phs->tile_size), width - 1);
	short tile_y1 = min(floor((A->aabb[3] - B->aabb[1]) / phs->tile_size), height - 1);
	for (int i = tile_x0; i <= tile_x1; i++) {
		for (int j = tile_y0; j <= tile_y1; j++) {
			int tile_idx = phs->bitmasktable[B->data[i + j * width]];
			if (tile_idx != TILE_EMPTY) {
				rect2 taabb = { A->aabb[0] - B->aabb[0] - i * phs->tile_size,  A->aabb[1] - B->aabb[1] - j * phs->tile_size, A->aabb[2] - B->aabb[0] - i * phs->tile_size,  A->aabb[3] - B->aabb[1] - j * phs->tile_size };
				int or , ol, od, ou;
				if (tile_idx == TILE_SOLID) {
					or = phs->tile_size - taabb[0] + 1;
					ol = taabb[2] + 1;
					od = phs->tile_size - taabb[1] + 1;
					ou = taabb[3] + 1;
				}
				else { //Slope
					or = bitmask_offset(phs->bitmasks[tile_idx], phs->tile_size, taabb, 1.0f, 0);
					ol = bitmask_offset(phs->bitmasks[tile_idx], phs->tile_size, taabb, -1.0f, 0);
					od = bitmask_offset(phs->bitmasks[tile_idx], phs->tile_size, taabb, 0, 1.0f);
					ou = bitmask_offset(phs->bitmasks[tile_idx], phs->tile_size, taabb, 0, -1.0f);
				}
				if (or < ol) xr = max(or , xr);
				else xl = max(ol, xl);
				if (od < ou) yd = max(od, yd);
				else yu = max(ou, yu);
			}
		}
	}
	if (xr || xl || yd || yu) {
		if ((xr != 0) ^ (xl != 0) && (yd != 0) ^ (yu != 0)) { //On point
			if (max(xr,xl) < max(yd, yu)) {
				pair->normal[0] = (xr > xl) ? -xr + residual(A->aabb[0]) : xl - (1 - residual(A->aabb[2]));
				pair->normal[1] = 0.0f;
			}
			else {
				pair->normal[0] = 0.0f;
				pair->normal[1] = (yd > yu) ? -yd + residual(A->aabb[1]) : yu - (1 - residual(A->aabb[3]));
			}
		}
		else if (xr && xl && yd && yu) { //In corner
			pair->normal[0] = (xr < xl) ? -xr + residual(A->aabb[0]) : xl - (1 - residual(A->aabb[2]));
			pair->normal[1] = (yd < yu) ? -yd + residual(A->aabb[1]) : yu - (1 - residual(A->aabb[3]));
		}
		else if (xr && xl) { //On ground
			pair->normal[0] = 0.0f;     
			pair->normal[1] = (yd > yu) ? -yd + residual(A->aabb[1]) : yu - (1 - residual(A->aabb[3]));
		}
		else if(yd && yu) { //On wall
			pair->normal[0] = (xr > xl) ? -xr + residual(A->aabb[0]) : xl - (1 - residual(A->aabb[2]));
			pair->normal[1] = 0.0f;
		}
		pair->penetration = vec2_len(pair->normal);
		vec2_normalize(pair->normal);

		return 1;
	}
	return 0;
}

int collision_solve(physics_t* phs, collision_t* pair) {
	if (pair->A->type == BODY_COLL) {
		int c = 0;
		if (pair->B->type == BODY_COLL)
			c = collision_bodyvsbody(phs, pair, pair->A, pair->B);
		else if (pair->B->type == CELL_COLL)
			c = collision_bodyvstile(phs, pair, pair->A, pair->B);
		
		if (c) 
			return (pair->normal[0] > 0) | (pair->normal[0] < 0) << 1 | (pair->normal[1] > 0) << 2 | (pair->normal[1] < 0) << 3;
		else 
			return 0;
	}
	return 0;
}

void collision_applyimpulse(collision_t* pair) {
	collider_t* A = pair->A;
	collider_t* B = pair->B;
	if (A->inv_mass + B->inv_mass <= EPSILON) {
		collision_infinitmass_correction(pair);
		return;
	}
	vec2 rv;
	collider_relativespeed(A, B, rv);
	float contactv = vec2_dot(rv, pair->normal);
	if (contactv > 0)
		return;

	float inv_mass = A->inv_mass + B->inv_mass;
	float j = -(1.0f + pair->bounce) * contactv / inv_mass;
	vec2 impulse = { -pair->normal[0] * j, -pair->normal[1] * j };

	if (A->type == BODY_COLL) 
		body_applyimpulse(A, impulse);
	if (B->type == BODY_COLL) 
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
	if (A->type == BODY_COLL) 
		body_applyimpulse(A, tan_impulse);
	if (B->type == BODY_COLL) 
		body_applyimpulse(B, (vec2) { -tan_impulse[0], -tan_impulse[1] });
}

void collision_correction(physics_t* phs, collision_t* pair) {
	body_t* A = pair->A;
	body_t* B = pair->B;
	float penetration_depth = max(pair->penetration - PENETRATION_SLOP, 0.0f) / (A->inv_mass + B->inv_mass) * PENETRATION_PERCENT;
	vec2 correction = { penetration_depth * pair->normal[0], penetration_depth * pair->normal[1] };
	vec2 dA = { A->inv_mass * correction[0], A->inv_mass * correction[1] };
	vec2 dB = { B->inv_mass * correction[0], B->inv_mass * correction[1] };
	if (A->type == BODY_COLL) 
		physics_movebody(phs, A, A->aabb[0] - dA[0], A->aabb[1] - dA[1], A->aabb[2] - A->aabb[0], A->aabb[3] - A->aabb[1]);
	if (B->type == BODY_COLL)
		physics_movebody(phs, B, B->aabb[0] + dB[0], B->aabb[1] + dB[1], B->aabb[2] - B->aabb[0], B->aabb[3] - B->aabb[1]);
}

void collision_infinitmass_correction(collision_t* pair) {
	body_t* A = pair->A;
	body_t* B = pair->B;
	if (A->type == BODY_COLL)
		A->speed[0] = 0.0f; A->speed[1] = 0.0f;
	if (B->type == BODY_COLL)
		B->speed[0] = 0.0f; B->speed[1] = 0.0f;
}