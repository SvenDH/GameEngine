#pragma once
#include "platform.h"
#include "texture.h"
#include "sprite.h"
#include "utils.h"
#include "math.h"

#include <stdint.h>
#include <lauxlib.h>

#define COMPONENT_MASK(name) (1 << comp_##name)

/* declare entity components */
typedef UID ent_id;
typedef uint8_t ent_type;

typedef struct {
	vec2 position;
	struct ent_transform* parent;
} ent_transform;

typedef struct {
	vec2 speed;
	vec2 acceletation;
} ent_physics;

//TODO: add vertices
typedef struct {
	Texture* texture;
	uint32_t color;
	float alpha;
	uint16_t index;
	int8_t offset[2];
	int8_t size[2];
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

void get_integer(lua_State* L, int* integer);
void set_integer(lua_State* L, int* integer, int i);

void get_vector(lua_State* L, vec2* vector);
void set_vector(lua_State* L, vec2* vector, int i);

void get_texture(lua_State* L, Texture** tex);
void set_texture(lua_State* L, Texture** tex, int i);

/* define types and name for entities and components */
#define ENTITY_DATA \
X(id,		ent_id) \
X(type,		ent_type) \
X(transform,ent_transform, \
	Y(position,		get_vector), \
	Z(position,		set_vector)) \
X(physics,	ent_physics, \
	Y(speed,		get_vector) \
	Y(acceletation,	get_vector), \
	Z(speed,		set_vector) \
	Z(acceletation,	set_vector)) \
X(sprite,	ent_sprite, \
	Y(texture,		get_texture), \
	Z(texture,		set_texture)) \
X(animation,ent_animation, , ) \
X(collider,	ent_collider, , )
//TODO: generate structs from this macro

typedef enum component_type {
#define X(name, type) comp_##name,
	ENTITY_DATA
#undef X
	NR_COMPS
};

#define X(name, type, getter, setter) \
int name##_index(lua_State *L); \
int name##_newindex(lua_State *L);
ENTITY_DATA
#undef X

#define COMP_MASK(name) (1 << comp_##name)
