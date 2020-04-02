#include "physics.h"

void collider_getaabb(Collider* col, Transform* t, rect2 aabb) {
	float x = t->position[0] - col->offset[0];
	float y = t->position[1] - col->offset[1];
	memcpy(aabb, (rect2) { x, y, x + col->size[0], y + col->size[1] }, sizeof(rect2));
}

bool _test_bodyvstile(rect2 A, rect2 B, byte* tiles, struct tile_info* tileinfo) {
	//TODO: This takes up much space
	int scratch_mask[MAX_TILE_SIZE * MAX_TILE_SIZE / (8 * sizeof(int))];
	int tile_size = tileinfo->size;
	uint16_t width = (B[2] - B[0]) / tile_size;
	uint16_t height = (B[3] - B[1]) / tile_size;
	rect2 tile_range = { floor((A[0] - B[0]) / tile_size), floor((A[1] - B[1]) / tile_size), floor((A[2] - B[0]) / tile_size), floor((A[3] - B[1]) / tile_size) };
	for (int j = max(tile_range[1], 0); j <= min(tile_range[3], height - 1); j++) {
		for (int i = max(tile_range[0], 0); i <= min(tile_range[2], width-1); i++) {
			tile_type tile_idx = tileinfo->tilemap[tiles[i + j * width]];
			if (tile_idx != TILE_EMPTY) {
				rect2 taabb = {A[0] - B[0] - i * tile_size, A[1] - B[1] - j * tile_size, A[2] - B[0] - i * tile_size, A[3] - B[1] - j * tile_size };
				if (tile_idx == TILE_SOLID)
					return true;
				else { //Slope
					bitmask_range(scratch_mask, tile_size, max(taabb[0], 0), max(taabb[1], 0), min(taabb[2] + 1, tile_size), min(taabb[3] + 1, tile_size));
					if (bitmask_overlap(tileinfo->bitmasks[tile_idx], scratch_mask, tile_size))
						return true;
				}
			}
		}
	}
	return false;
}

bool _test_bodyvsbody(rect2 A, rect2 B) {
	return aabb_overlap(A, B);
}

bool pair_overlap(space_t* spc, entity_t body, entity_t other) {
	world_t* wld = world_instance();
	ENTITY_GETCOMP(wld, Collider, A, body);
	ENTITY_GETCOMP(wld, Collider, B, other);
	ENTITY_GETCOMP(wld, Transform, tA, body);
	ENTITY_GETCOMP(wld, Transform, tB, other);
	ENTITY_GETCOMP(wld, Tiles, tileB, other);
	rect2 aabbA, aabbB;
	collider_getaabb(A, tA, aabbA);
	collider_getaabb(B, tB, aabbB);
	//TODO: check masks
	if (!tileB) {
		return _test_bodyvsbody(aabbA, aabbB);
	}
	else {
		return _test_bodyvstile(aabbA, aabbB, tileB, &spc->tiles);
	}
	//TODO: check other way around
}

SYSTEM_ENTRY(collider_new) {
	space_t* spc = task->data;
	COMPARRAY_GET(EntityId, ent);
	COMPARRAY_GET(Collider, col);
	for (int i = task->start; i < task->end; i++) {
		col[i].offset[0] = 0.0f;
		col[i].offset[1] = 0.0f;
		col[i].layer = 0;
		col[i].mask = 1;
		col[i].size[0] = 1.0f;
		col[i].size[1] = 1.0f;
		space_create(spc, ent[i].id);
	}
}

SYSTEM_ENTRY(collider_delete) {
	space_t* spc = task->data;
	COMPARRAY_GET(EntityId, ent);
	for (int i = task->start; i < task->end; i++) {
		space_remove(spc, ent[i].id);
	}
}

SYSTEM_ENTRY(collider_update) {
	space_t* spc = task->data;
	COMPARRAY_GET(EntityId, ent);
	COMPARRAY_GET(Transform, t);
	COMPARRAY_GET(Collider, col);
	for (int i = task->start; i < task->end; i++) {
		float x = t[i].position[0] - col[i].offset[0];
		float y = t[i].position[1] - col[i].offset[1];
		rect2 aabb;
		collider_getaabb(&col[i], &t[i], aabb);
		space_move(spc, ent[i].id, aabb, true);
	}
}
