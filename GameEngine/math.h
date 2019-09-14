#pragma once
#include "types.h"
#include "utils.h"

#include <math.h>
#include <float.h>
#include <lauxlib.h>

#define Vector_mt "vector"

#define MAT3_ZERO { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }
#define MAT3_IDENTITY { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }

/*
//TODO: make this a macro
inline void transform_mesh(mat3 t, pos_vertex_t* dest, pos_vertex_t* src, size_t size) {
	for (int i = 0; i < size; i++) {
		dest[i].pos[0] = (t[0][0] * src[i].pos[0]) + (t[1][0]* src[i].pos[1]) + t[2][0];
		dest[i].pos[1] = (t[0][1] * src[i].pos[0]) + (t[1][1] * src[i].pos[1]) + t[2][1];
		dest[i].tex[0] = src[i].tex[0];
		dest[i].tex[1] = src[i].tex[1];
	}
}
*/

inline void transform_identity(mat3 t) {
	memset(t, 0, sizeof(mat3));
	t[0][0] = t[1][1] = t[2][2] = 1.0f;
}

inline void tranform_ortho(mat3 t, float left, float right, float bottom, float top) {
	memset(t, 0, sizeof(mat3));
	t[0][0] = 2.f / (right - left);
	t[1][1] = 2.f / (top - bottom);

	t[2][0] = -(right + left) / (right - left);
	t[2][1] = -(top + bottom) / (top - bottom);

	t[2][2] = -1.f;
}

inline void transform_mul(mat3 d, mat3 t, mat3 m) {
	d[0][0] = (t[0][0] * m[0][0]) + (t[1][0] * m[0][1]) + (t[2][0] * m[0][2]);
	d[1][0] = (t[0][0] * m[1][0]) + (t[1][0] * m[1][1]) + (t[2][0] * m[1][2]);
	d[2][0] = (t[0][0] * m[2][0]) + (t[1][0] * m[2][1]) + (t[2][0] * m[2][2]);

	d[0][1] = (t[0][1] * m[0][0]) + (t[1][1] * m[0][1]) + (t[2][1] * m[0][2]);
	d[1][1] = (t[0][1] * m[1][0]) + (t[1][1] * m[1][1]) + (t[2][1] * m[1][2]);
	d[2][1] = (t[0][1] * m[2][0]) + (t[1][1] * m[2][1]) + (t[2][1] * m[2][2]);

	d[0][2] = (t[0][2] * m[0][0]) + (t[1][2] * m[0][1]) + (t[2][2] * m[0][2]);
	d[1][2] = (t[0][2] * m[1][0]) + (t[1][2] * m[1][1]) + (t[2][2] * m[1][2]);
	d[2][2] = (t[0][2] * t[2][0]) + (t[1][2] * m[2][1]) + (t[2][2] * m[2][2]);
}

inline void transform_set(mat3 t, float x, float y, float angle, float sx, float sy, float ox, float oy, float kx, float ky) {
	float c = cosf(angle), s = sinf(angle);
	// |1    x| |c -s  | |sx     | | 1 ky  | |1   -ox|
	// |  1  y| |s  c  | |   sy  | |kx  1  | |  1 -oy|
	// |     1| |     1| |      1| |      1| |     1 |
	//   move    rotate    scale     skew      origin
	t[0][0] = c * sx - ky * s * sy; // = a
	t[0][1] = s * sx + ky * c * sy; // = b
	t[1][0] = kx * c * sx - s * sy; // = c
	t[1][1] = kx * s * sx + c * sy; // = d
	t[2][0] = x - ox * t[0][0] - oy * t[1][0];
	t[2][1] = y - ox * t[0][1] - oy * t[1][1];

	t[0][2] = t[1][2] = 0.0f;
	t[2][2] = 1.0f;
}

int openlib_Math(lua_State* L);