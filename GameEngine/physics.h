#pragma once
#include "data.h"
#include "utils.h"
#include "math.h"
#include "ecs.h"

#define Physics_mt "physics"

#define BITMASKTABLE_SIZE 256
#define TILE_SIZE 16
#define PHYSICS_ITERATIONS 10

#define PENETRATION_PERCENT 0.9f
#define PENETRATION_SLOP 0.05f

#define BIGCOLLIDERSIZE 512

enum collider_enum {
	CELL_COLL,
	BODY_COLL
};

enum tile_enum {
	TILE_EMPTY,
	TILE_SOLID,
	TILE_LSLOPE,
	TILE_RSLOPE,
	NR_TILES
};

typedef struct {
	object_allocator_t bodies;
	object_allocator_t nodes;
	object_allocator_t pairs;
	object_allocator_t cells;

	hashmap_t objectmap;
	hashmap_t bodygrid;
	hashmap_t largeobjects;

	vec2 gravity;
	int tile_size;

	size_t bitmasktable[BITMASKTABLE_SIZE];
	int bitmasks[NR_TILES][TILE_SIZE * TILE_SIZE / (8 * sizeof(int))];
	//0: EMPTY
	//1: EMPTY
	//2: SOLID
	//3: LSLOPE
	//4: RSLOPE
	//5:  etc...
} physics_t;

typedef struct {
	uint64_t id;
	int type;
	float inv_mass, friction, bounce;
	hashmap_t pairs;
	int layer;
	int mask;
	int _static;
	rect2 aabb;
} collider_t;

typedef struct {
	collider_t;
	unsigned char data[2];
} tilecell_t;

typedef struct {
	collider_t;
	vec2 speed;
	vec2 force;
} body_t;

typedef struct {
	short rc;
	int colliding;
	collider_t* A;
	collider_t* B;
	vec2 normal;
	float penetration, friction, bounce;
} collision_t;

typedef struct {
	hashmap_t objects;
} gridnode_t;

void physics_init(physics_t* phs, const char* name);
void physics_step(physics_t* phs, double dt);

int cell_update(system_t* system, event_t evt);
int body_update(system_t* system, event_t evt);
int physics_update(system_t* system, event_t evt);

tilecell_t* cell_new(physics_t* phs, UID id);
void cell_delete(physics_t* phs, UID id);
tilecell_t* cell_get(physics_t* phs, UID id);
void cell_set(physics_t* phs, UID id, const char* data, size_t offset, size_t len);


body_t* physics_newbody(physics_t* phs, UID id);
void physics_deletebody(physics_t* phs, UID id);
void physics_colliderinit(physics_t* phs, collider_t* body, UID id, int type, int is_static);
void physics_colliderdelete(physics_t* phs, collider_t* body);
void physics_integrateforces(physics_t* phs, body_t* body, float dt);
void physics_integratespeed(physics_t* phs, body_t* body, float dt);
void physics_movebody(physics_t* phs, collider_t* body, float x, float y, float width, float height);
void _physics_bodyinsert(physics_t* phs, collider_t* body, rect2 aabb);
void _physics_bodyremove(physics_t* phs, collider_t* body, rect2 aabb);
void _physics_pair(physics_t* phs, collider_t* body, collider_t* other);
void _physics_unpair(physics_t* phs, collider_t* body, collider_t* other);

void physics_onstartcollide(physics_t* phs, body_t* body, body_t* other);
void physics_onstopcollide(physics_t* phs, body_t* body, body_t* other);

body_t* body_get(physics_t* phs, UID id);
void body_set(body_t* body, float mass, float friction, float bounce);
void body_applyforce(body_t* body, vec2 force);
void body_applyimpulse(body_t* body, vec2 impulse);

void collision_init(collision_t* pair, collider_t* A, collider_t* B);
void collision_update(collision_t* pair, float grav_len);
int collision_solve(physics_t* phs, collision_t* pair);
void collision_applyimpulse(collision_t* pair);
void collision_correction(physics_t* phs, collision_t* pair);
void collision_infinitmass_correction(collision_t* pair);

inline GLOBAL_SINGLETON(physics_t, physics, "Physics");

int openlib_Physics(lua_State* L);


inline void collider_relativespeed(collider_t* A, collider_t* B, vec2 rv) {
	rv[0] = 0.0f;
	rv[1] = 0.0f;
	if (A->type == BODY_COLL) {
		rv[0] -= ((body_t*)A)->speed[0];
		rv[1] -= ((body_t*)A)->speed[1];
	}
	if (B->type == BODY_COLL) {
		rv[0] += ((body_t*)B)->speed[0];
		rv[1] += ((body_t*)B)->speed[1];
	}
}

inline void bitmask_print(int* mask, size_t size) {
	for (int j = 0; j < size; j++) {
		for (int i = 0; i < size; i++) {
			int m = ((j * size) + i) / (sizeof(int) * 8);
			int n = ((j * size) + i) % (sizeof(int) * 8);
			int r = (mask[m] >> n) & 1;
			printf("%i ", r);
		}
		printf("\n");
	}
}

inline int bitmask_overlap(int* mask_a, int* mask_b, size_t size) {
	for (int i = 0; i < (size * size) / (sizeof(int) * 8); i++) {
		if (mask_a[i] & mask_b[i])
			return 1;
	}
	return 0;
}

inline void bitmask_empty(int* mask, size_t size) {
	for (int i = 0; i < (size * size) / (sizeof(int) * 8); i++)
		mask[i] = 0;
}

inline void bitmask_solid(int* mask, size_t size) {
	for (int i = 0; i < (size * size) / (sizeof(int) * 8); i++)
		mask[i] = ~0;
}

inline void bitmask_slope(int* mask, size_t size, int h_mirror, int v_mirror) {
	for (int j = 0; j < size; j++) {
		for (int i = 0; i < size; i++) {
			int m = ((j * size) + i) / (sizeof(int) * 8);
			int n = ((j * size) + i) % (sizeof(int) * 8);
			if (!v_mirror && !h_mirror && i >= j ||
				!v_mirror && h_mirror && size - 1 - i >= j ||
				v_mirror && !h_mirror && i >= size - 1 - j ||
				v_mirror && h_mirror && size - 1 - i >= size - 1 - j)
				mask[m] |= 1 << n;
			else
				mask[m] &= ~(1 << n);
		}
	}
}

inline void bitmask_range(int* mask, size_t size, int start_x, int start_y, int end_x, int end_y) {
	const int pr = (sizeof(int) * 8) / size;
	int bytes = setbitrange(start_x + 1, end_x);
	for (int i = 0; i < (size * size) / (sizeof(int) * 8); i++) {
		mask[i] = 0;
		for (int j = 0; j < pr; j++) {
			int y = i * pr + j;
			if (start_y <= y && y < end_y)
				mask[i] |= bytes << (j * size);
		}
	}
}

inline int bitmask_offset(int* mask, size_t size, rect2 aabb, float dx, float dy) {
	int scratch_mask[TILE_SIZE * TILE_SIZE / (8 * sizeof(int))];
	int offset = 0;
	for (int i = 0; i < 2 * size; i++) {
		int start_x = max(aabb[0] + offset * dx, 0);
		int start_y = max(aabb[1] + offset * dy, 0);
		int end_x = min(aabb[2] + offset * dx + 1, size);
		int end_y = min(aabb[3] + offset * dy + 1, size);
		bitmask_range(scratch_mask, size, start_x, start_y, end_x, end_y);
		if (!bitmask_overlap(mask, scratch_mask, size)) 
			return offset;

		offset ++;
	}
	return size;
}



