#pragma once
#include "types.h"
#include "event.h"
#include "data.h"
#include "utils.h"
#include "resource.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Entity_mt		"entity"
#define Chunk_mt		"chunk"

#define TILE_ROWS 8
#define TILE_COLS 8

static int ecs_component_counter = 0;

/* define types and name for entities and components */
#define ENTITY_DATA \
X(id,		1, \
	Y(entity_t, uid)) \
X(container,1, \
	Y(chunk_t,	children)) \
X(transform,1, \
	Y(vec2,		position)) \
X(physics,	1, \
	Y(vec2,		speed) \
	Y(vec2,		force) \
	Y(float,	mass) \
	Y(float,	friction) \
	Y(float,	bounce) \
	Y(short,	flags)) \
X(sprite,	1, \
	Y(rid_t,	texture) \
	Y(short,	index) \
	Y(int,		color) \
	Y(float,	alpha) \
	Y(vec2,		offset)) \
X(animation,1, \
	Y(short,	start_index) \
	Y(short,	end_index) \
	Y(float,	index) \
	Y(float,	speed) \
	Y(short,	flags)) \
X(collider,	1, \
	Y(vec2,		offset) \
	Y(vec2,		size)) \
X(tiles,	1, \
	Y(byte,		data, [TILE_ROWS * TILE_COLS])) \
X(camera,	1, \
	Y(vec2,		size) \
	Y(rid_t,	shader) \
	Y(rid_t,	texture)) \
X(disabled,	0, Y(byte,no,[1]))


#define Y(_type, _name, _size) _type _name _size;
#define X(_name, _size, _attr) \
static int T##_name = __COUNTER__; \
typedef struct { \
	_attr \
} ent_##_name; \
int w_##_name##_index(lua_State * L); \
int w_##_name##_newindex(lua_State *L); \
void _name##_init(ent_##_name* comp); \
void _name##_delete(ent_##_name* comp);
ENTITY_DATA
#undef X

enum component_type {
#define X(name, size, attr) comp_##name,
	ENTITY_DATA
#undef X
	NR_COMPS
};

enum component_masks {
#define X(name, size, attr) name = 1 << comp_##name,
	ENTITY_DATA
#undef X
};

static size_t component_sizes[sizeof(type_t) * 8] = {
#define X(name, size, attr) sizeof(ent_##name) * size,
ENTITY_DATA
#undef X
};


static entity_t			default_id = 0;
static char				default_disabled = '\0';
static ent_container	default_container = { .children = 0 };
static ent_transform	default_transform = { .position = { 0.0f, 0.0f } };
static ent_physics		default_physics = { .speed = { 0.0f, 0.0f },.force = { 0.0f, 0.0f },.mass = 0.0f,.friction = 0.0f,.bounce = 0.0f,.flags = 0 };
static ent_sprite		default_sprite = { .texture = {.type = RES_TEXTURE, .name = 0}, .index = 0,.color = 0xffffff,.alpha = 1.0f,.offset = { 0.0f, 0.0f } };
static ent_animation	default_animation = { .start_index = 0,.end_index = 1,.index = 0.0f,.speed = 1.0f,.flags = 0 };
static ent_collider		default_collider = { .offset = { 0.0f, 0.0f },.size = { 1.0f, 1.0f } };
static ent_tiles		default_tiles = { .data = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, } };
static ent_camera		default_camera = { .size = { 0.f, 0.f }, .shader = {.type = RES_SHADER, .name = 0 }, .texture = {.type = RES_TEXTURE, .name = 0 } };

static void* component_default[] = {
#define X(name, type, size, attr) &default_##name,
ENTITY_DATA
#undef X
};

#define entity_gettype(_wld, _ent) \
	entity_to_chunk((_wld), (_ent), NULL)->type

#define entity_getparent(_wld, _ent) \
	entity_to_chunk((_wld), (_ent), NULL)->parent

#define entity_changetype(_wld, _ent, _new_type) \
	entity_change((_wld), (_ent), entity_getparent(_wld, _ent), (_new_type))

#define entity_changeparent(_wld, _ent, _new_parent) \
	entity_change((_wld), (_ent), (_new_parent), entity_gettype(_wld, _ent))

#define archetype_foreach(_wld, _arch, _chunk) \
	for (chunk_t _id = (_arch)->chunk_list; \
		_id, _chunk = chunk_find((_wld), _id); \
		_id = (_chunk)->next_chunk)

#define children_foreach(_wld, _first, _chunk) \
	for (chunk_t _id = (_first); \
		_id, _chunk = chunk_find((_wld), _id); \
		_id = (_chunk)->next_children)


typedef struct system_t system_t;

typedef struct {
//	uint32_t _magic;
	chunk_t id;					//4
	type_t type;				//8
	entity_t parent;			//16
	uint32_t ent_count;			//20
	uint32_t max_ents;			//24
	chunk_t next_children;		//28
	chunk_t prev_children;		//32
	chunk_t next_chunk;			//36
	chunk_t prev_chunk;			//40
	char data[4];
} chunk_data;

typedef struct {
	type_t type;
	int max_ents;
	uint16_t sizes[MAX_COMPS];
	uint16_t offsets[MAX_COMPS];
	chunk_t chunk_list;
	int chunk_count;
} type_data;

typedef union {
	struct {
		type_t react;
		type_t and;
		type_t not;
	};
	key_t value;
} group_t;

typedef struct system_t {
	listnode_t;
	group_t mask;
	struct world_t* world;
	evt_callback_t cb;
	uint32_t evt;
	hashmap_t archetypes;

	void* data;
};

typedef struct {
	object_allocator_t chunks;
	object_allocator_t archetypes;
	object_allocator_t systems;

	hashmap_t entity_map;
	hashmap_t chunk_map;
	hashmap_t archetype_map;
	listnode_t system_list;

	int chunk_index;
} world_t;

void world_init(world_t* world, const char* name);
void ecs_addcomponents_cb(world_t* wld, entity_t ent, type_t mask);
void ecs_delcomponents_cb(world_t* wld, entity_t ent, type_t mask);

chunk_data* chunk_new(world_t* wld, type_t type, entity_t parent);
chunk_data* chunk_find(world_t* wld, chunk_t id);
chunk_data* chunk_get(world_t* wld, type_t type, entity_t parent, int create);
void chunk_link(world_t* wld, chunk_data* chunk);
void chunk_relink(world_t* wld, chunk_data* chunk);
int chunk_change(world_t* wld, chunk_data* chunk, type_t new_type, entity_t new_parent);
void chunk_delete(world_t* wld, chunk_data* chunk);
int chunk_componentcopy(chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src);
int chunk_componentmove(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src);
void* chunk_get_componentarray(chunk_data* chunk, int component);
void* chunk_get_component(chunk_data* chunk, int component, int index);

chunk_data* component_to_chunk(void* comp_ptr, int comp_type, int* index);

type_data* archetype_new(world_t* wld, type_t components);
type_data* archetype_get(world_t* wld, type_t type, int create);
void archetype_delete(world_t* wld, type_data* archetype);

entity_t entity_new(world_t* wld, entity_t id, type_t type, entity_t parent);
entity_t* entity_get(world_t* wld, entity_t id);
void* entity_getcomponent(world_t* wld, entity_t ent, int component);
int entity_move(world_t* wld, entity_t ent, chunk_data* dest_chunk, int dest);
int entity_change(world_t* wld, entity_t ent, entity_t new_parent, type_t new_type);
chunk_data* entity_to_chunk(world_t* wld, entity_t ent, int* index);
void entity_delete(world_t* wld, entity_t ent);

system_t* system_new(world_t* wld, evt_callback_t cb, type_t react_mask, type_t and_mask, type_t not_mask, int evt, void* data);
void system_delete(world_t* wld, system_t* system);
int system_update(world_t* wld, event_t evt);
int system_listener(world_t* wld, event_t evt);
int ecs_debug_log(world_t* wld, event_t evt);

inline GLOBAL_SINGLETON(world_t, world, "World");

int openlib_ECS(lua_State* L);

int debug_system(system_t* system, event_t evt);

inline void ecs_addcomponents_cb(world_t* wld, entity_t ent, type_t mask) {
	for (int i = 0; i < NR_COMPS; i++)
		if ((1 << i) & mask)
			event_post(eventhandler_instance(),
			(event_t) {
			.type = on_addcomponent, .p0.data = i, .p1.data = ent
		});
}

inline void ecs_delcomponents_cb(world_t* wld, entity_t ent, type_t mask) {
	for (int i = 0; i < NR_COMPS; i++)
		if ((1 << i) & mask)
			event_post(eventhandler_instance(),
			(event_t) {
			.type = on_delcomponent, .p0.data = i, .p1.data = ent
		});
}
