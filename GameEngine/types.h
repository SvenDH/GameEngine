#pragma once
#include "platform.h"
#include <stdint.h>
#include <uv.h>
#include <lua.h>

#define GLOBAL_SINGLETON(_type, _fname, _name) \
_type* _fname##_instance() { \
	static _type* manager = NULL; \
	if (!manager) { \
		manager = malloc(sizeof(_type)); \
		_fname##_init(manager, _name); \
	} \
	return manager; \
}

typedef uint8_t byte;
typedef unsigned int uint;
typedef unsigned short ushort;

typedef uint64_t UID; //64 bit number
typedef uint64_t key_t;

typedef uint64_t entity_t; //TODO: add index, user-id and salt
typedef uint32_t type_t;
typedef uint32_t chunk_t;

typedef struct {
	int t;
	union {
		uint64_t data;
		double nr;
		void* ptr;
	};
} variant_t;

typedef char color_t[4];

typedef float vec2[2];
typedef float vec3[3];
typedef vec3  mat3[3];

typedef float rect2[4];

typedef struct {
	mat3 model;
} pos_vertex_t;

typedef struct {
	color_t col;
	uint32_t idx;
} col_vertex_t;
