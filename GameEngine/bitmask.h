#pragma once
#include "utils.h"

#define MAX_TILE_SIZE 256

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
	int scratch_mask[MAX_TILE_SIZE * MAX_TILE_SIZE / (8 * sizeof(int))];
	int offset = 0;
	for (int i = 0; i < 2 * size; i++) {
		int start_x = max(aabb[0] + offset * dx, 0);
		int start_y = max(aabb[1] + offset * dy, 0);
		int end_x = min(aabb[2] + offset * dx + 1, size);
		int end_y = min(aabb[3] + offset * dy + 1, size);
		bitmask_range(scratch_mask, size, start_x, start_y, end_x, end_y);
		if (!bitmask_overlap(mask, scratch_mask, size))
			return offset;

		offset++;
	}
	return size;
}