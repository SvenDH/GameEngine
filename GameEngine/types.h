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

typedef uint64_t UID; //64 bit number
typedef uint32_t event_t;
typedef uint64_t key_t;
typedef char color_t[4];

typedef float vec2[2];
typedef float vec3[3];
typedef vec3  mat3[3];

typedef struct {
	mat3 model;
} pos_vertex_t;

typedef struct {
	color_t col;
	uint32_t idx;
} col_vertex_t;

typedef struct {
	size_t size;
	char name[16];
} TypeMetaData;
