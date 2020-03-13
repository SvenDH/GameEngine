#include "physics.h"

tilecell_t* cell_new(physics_t* phs, UID id) {
	tilecell_t* cell = object_alloc(&phs->cells, sizeof(tilecell_t) + TILE_COLS * TILE_ROWS - 2);
	physics_colliderinit(phs, cell, id, CELL_COLL, 1);
	return cell;
}

void cell_delete(physics_t* phs, UID id) {
	tilecell_t* cell = hashmap_get(&phs->objectmap, id);
	physics_colliderdelete(phs, cell);
	object_free(&phs->cells, cell);
}

tilecell_t* cell_get(physics_t* phs, UID id) {
	tilecell_t* cell = hashmap_get(&phs->objectmap, id);
	assert(cell->type == CELL_COLL);
	return cell;
}

void cell_set(physics_t* phs, UID id, const char* data, size_t offset, size_t len) {
	tilecell_t* cell = hashmap_get(&phs->objectmap, id);
	assert(cell->type == CELL_COLL);
	//Make sure we do not write beyond buffer
	if (data) memcpy(cell->data, data, len);
}