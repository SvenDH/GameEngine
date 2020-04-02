#include "graphics.h"
#include "physics.h"

SYSTEM_ENTRY(sprite_new) {
	graphics_t* gfx = task->data;
	COMPARRAY_GET(EntityId, ent);
	COMPARRAY_GET(Sprite, spr);
	for (int i = task->start; i < task->end; i++) {
		spr[i].texture = gfx->default_texture;
		spr[i].index = 0;
		spr[i].color = WHITE;
		spr[i].alpha = 1.0;
		spr[i].offset[0] = 0.0f;
		spr[i].offset[1] = 0.0f;
		spr[i].size[0] = 0.0f;
		spr[i].size[1] = 0.0f;
		spr[i].depth = 0;
		space_create(&gfx->default_scene, ent[i].id);
	}
}

SYSTEM_ENTRY(sprite_delete) {
	graphics_t* gfx = task->data;
	COMPARRAY_GET(EntityId, ent);
	for (int i = task->start; i < task->end; i++) {
		space_remove(&gfx->default_scene, ent[i].id);
	}
}

SYSTEM_ENTRY(sprite_update) {
	graphics_t* gfx = task->data;
	COMPARRAY_GET(EntityId, ent);
	COMPARRAY_GET(Transform, t);
	COMPARRAY_GET(Sprite, spr);
	COMPARRAY_GET(Tiles, tile);
	for (int i = task->start; i < task->end; i++) {
		float x = t[i].position[0] - spr[i].offset[0];
		float y = t[i].position[1] - spr[i].offset[1];
		float xsize = spr[i].size[0];
		float ysize = spr[i].size[1];
		if (tile) {
			xsize *= TILE_ROWS;
			ysize *= TILE_COLS;
		}
		rect2 aabb = { x, y, x + xsize, y + ysize };
		space_move(&gfx->default_scene, ent[i].id, aabb, false);
	}
}

SYSTEM_ENTRY(animation_update) {
	double dt = task->delta_time;
	COMPARRAY_GET(Sprite, spr);
	COMPARRAY_GET(Animation, anim);
	for (int i = task->start; i < task->end; i++) {
		anim[i].index += anim[i].speed * dt;

		if (anim[i].speed > 0 && (
			anim[i].index > anim[i].end_index ||
			anim[i].index < anim[i].start_index))
			anim[i].index = anim[i].start_index;
		else if (anim[i].speed < 0 && (
			anim[i].index < anim[i].start_index ||
			anim[i].index >= anim[i].end_index))
			anim[i].index = anim[i].end_index - 1;

		spr[i].index = anim[i].index;
	}
}