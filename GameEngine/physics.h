#pragma once
#include "data.h"
#include "utils.h"
#include "math.h"
#include "ecs.h"
#include "bitmask.h"

#define Physics_mt "physics"

#define PHYSICS_ITERATIONS 10

#define PENETRATION_PERCENT 0.9f
#define PENETRATION_SLOP 0.05f

#define DEFAULT_TILE_SIZE 16
#define BITMASKTABLE_SIZE 256
#define MAX_SPACES 8

typedef struct {
	byte data[TILE_ROWS * TILE_COLS];
} COMP_DECLARE(Tiles);

typedef enum {
	TILE_EMPTY,
	TILE_SOLID,
	TILE_LSLOPE,
	TILE_RSLOPE,
	NR_TILES
} tile_type;

struct tile_info {
	uint8_t size;
	tile_type tilemap[BITMASKTABLE_SIZE];
	//TODO: this takes up more than 8000 bytes:
	int bitmasks[NR_TILES][MAX_TILE_SIZE * MAX_TILE_SIZE / (8 * sizeof(int))];
};

typedef struct {
	object_allocator_t nodes;
	object_allocator_t objects;
	object_allocator_t pairs;
	hashmap_t objectmap;
	hashmap_t bodygrid;
	struct tile_info tiles;
	int pass;
	//0: EMPTY
	//1: EMPTY
	//2: SOLID
	//3: LSLOPE
	//4: RSLOPE
	//5:  etc...
} space_t;

void space_new(space_t* phs, uint16_t tile_size, int* tiletable);
void space_delete(space_t* spc);
void space_create(space_t* spc, UID id);
void space_remove(space_t* spc, UID id);
void space_move(space_t* spc, UID id, rect2 aabb, bool collide);
int space_cullray(space_t* spc, vec2 begin, vec2 end, UID* results, int max_results);
int space_cullaabb(space_t* spc, rect2 aabb, UID* results, int max_results);

typedef struct {
	vec2 gravity;
	space_t default_space;
} physics_t;

void physics_init(physics_t* phs, const char* name);
entity_t  physics_onstartcollide(entity_t body, entity_t other);
void physics_onstopcollide(entity_t pair);

typedef struct {
	vec2 speed;
	vec2 force;
	float friction;
	float bounce;
	float mass;
	float inv_mass;
} COMP_DECLARE(Body);

void body_new(task_data* task);
void body_set(task_data* task);
void body_integrateforces(task_data* task);
void body_integratespeed(task_data* task);
void body_applyforce(Body* body, vec2 force);
void body_applyimpulse(Body* body, vec2 impulse);

typedef struct {
	vec2 offset;
	vec2 size;
	uint32_t layer;
	uint32_t mask;
} COMP_DECLARE(Collider);

void collider_new(task_data* task);
void collider_delete(task_data* task);
void collider_update(task_data* task);
void collider_getaabb(Collider* col, Transform* t, rect2 aabb);

typedef struct {
	entity_t A, B;
	vec2 normal;
	float penetration, friction, bounce;
} COMP_DECLARE(Collision);

void contraint_update(task_data* task);
void contraint_solve(task_data* task);
void contraint_correction(task_data* task);

bool pair_overlap(space_t* spc, entity_t body, entity_t other);

inline GLOBAL_SINGLETON(physics_t, physics, "Physics");

int openlib_Physics(lua_State* L);

static tile_type default_tilemap[BITMASKTABLE_SIZE] = { 
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	1,1,1,1,2,3,0,0,
	0,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0 };