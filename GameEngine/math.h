#pragma once
#include "types.h"
#include "utils.h"

#include <math.h>
#include <float.h>
#include <lauxlib.h>

#define Vector_mt "vector"

#define VEC2_ZERO (vec2){ 0.0f, 0.0f }
#define MAT3_ZERO { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }
#define MAT3_IDENTITY { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
#define NULL_RECT (rect2){0.0f, 0.0f, 0.0f, 0.0f}
#define EPSILON 0.0001f

#define SWAP(_x,_y) { const int _a = min(_x, _y); \
                    const int _b = max(_x, _y); \
                    _x = _a; _y = _b; }

#define SGN(_v) (((_v) < 0) ? (-1.0) : (+1.0))

inline float real_abs(float n) {
	return (n > 0) ? n : -n;
}

inline float vec2_len(vec2 v) {
	return sqrt(v[0] * v[0] + v[1] * v[1]);
}

inline void vec2_normalize(vec2 v) {
	float len = vec2_len(v);
	if (len > EPSILON) {
		float invLen = 1.0f / len;
		v[0] *= invLen;
		v[1] *= invLen;
	}
}

inline float vec2_dot(const vec2 a, const vec2 b) {
	return a[0] * b[0] + a[1] * b[1];
}

inline float vec2_cross(const vec2 a, const vec2 b) {
	return a[0] * b[1] + a[1] * b[0];
}

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

inline float residual(float p) {
	return p - floor(p);
}

inline void aabb_zero(rect2 a) {
	a[0] = 0.0f; a[1] = 0.0f; a[2] = 0.0f; a[3] = 0.0f;
}

inline bool aabb_overlap(rect2 a, rect2 b) {
	return !((int)a[0] > (int)b[2]
		|| (int)a[1] > (int)b[3]
		|| (int)a[2] < (int)b[0]
		|| (int)a[3] < (int)b[1]);
}

inline bool aabb_segment(rect2 a, vec2 from, vec2 to, vec2 normal, vec2 pos) {
	float min = 0, max = 1;
	int axis = 0;
	float sign = 0;
	for (int i = 0; i < 2; i++) {
		float seg_from = from[i];
		float seg_to = to[i];
		float box_begin = a[i];
		float box_end = a[2 + i];
		float cmin, cmax, csign;
		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin)
				return false;
			float length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;
		}
		else {
			if (seg_to > box_end || seg_from < box_begin)
				return false;
			float length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}
		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max)
			max = cmax;
		if (max < min)
			return false;
	}
	vec2 rel = { to[0] - from[0], to[1] - from[1] };
	if (normal) {
		normal[axis] = sign;
		normal[1-axis] = 0;
	}
	if (pos) {
		pos[0] = from[0] + rel[0] * min;
		pos[1] = from[1] + rel[1] * min;
	}
	return true;
}

inline void aabb_correct(rect2 a) {
	SWAP(a[0], a[2]);
	SWAP(a[1], a[3]);
}

int openlib_Math(lua_State* L);