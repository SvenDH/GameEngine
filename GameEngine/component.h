#pragma once
#include "platform.h"
#include "graphics.h"
#include "utils.h"
#include "resource.h"
#include "math.h"
#include "types.h"

#include <lualib.h>
#include <lauxlib.h>

/* declare entity components */
typedef struct {
	vec2 position;
} ent_transform;

typedef struct {
	uint32_t children;
	uint16_t nr_children;
} ent_container;

typedef struct {
	vec2 speed;
	vec2 acceletation;
} ent_physics;

typedef struct {
	rid_t texture;
	vec2 offset;
	uint16_t index;
	uint32_t color;
	float alpha;
} ent_sprite;

typedef struct {
	uint16_t index_start;
	uint16_t index_end;
	int8_t speed;
} ent_animation;

typedef struct {
	int8_t offset[2];
	uint8_t size[2];
	int16_t prev;
	int16_t next;
} ent_collider;

/* define types and name for entities and components */
#define ENTITY_DATA \
X(id,		entity_t,		sizeof(entity_t)) \
X(container,ent_container,	sizeof(ent_container), ) \
X(transform,ent_transform,	sizeof(ent_transform), \
	Y(position,		vector)) \
X(physics,	ent_physics,	sizeof(ent_physics), \
	Y(speed,		vector) \
	Y(acceletation,	vector)) \
X(sprite,	ent_sprite,		sizeof(ent_sprite), \
	Y(texture,		texture) \
	Y(offset,		vector) \
	Y(index,		integer) \
	Y(alpha,		float)) \
X(animation,ent_animation,	sizeof(ent_animation), ) \
X(collider,	ent_collider,	sizeof(ent_collider), ) \
X(disabled,	void,			0, )
//TODO: generate structs from this macro

typedef enum component_type {
#define X(name, type) comp_##name,
	ENTITY_DATA
#undef X
	NR_COMPS
};

typedef enum component_masks {
#define X(name) name = 1 << comp_##name,
	ENTITY_DATA
#undef X
};

#define X(name) \
int name##_index(lua_State *L); \
int name##_newindex(lua_State *L);
ENTITY_DATA
#undef X
