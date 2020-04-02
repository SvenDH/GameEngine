#include "data.h"
#include "math.h"

typedef hashmap_t gridnode_t;

typedef struct {
	hashmap_t paired;
	rect2 aabb;
	UID id;
	int pass;
} element_t;

typedef struct {
	short rc;
	int colliding;
	UID data;
} pair_t;

void hashgrid_new(hashgrid_t* spc, uint16_t tile_size, pair_cb pair, unpair_cb unpair, void* ud) {
	object_init(&spc->nodes, "hashgrid.nodes", sizeof(gridnode_t), 64, 8);
	object_init(&spc->pairs, "hashgrid.pairs", sizeof(pair_t), 64, 8);
	object_init(&spc->objects, "hashgrid.objects", sizeof(element_t), 64, 8);
	//TODO: add big object list or grid
	hashmap_init(&spc->bodygrid);
	hashmap_init(&spc->objectmap);
	spc->tile_size = tile_size;
	spc->pass = 0;
	spc->pair = pair;
	spc->unpair = unpair;
	spc->ud = ud;
}

void hashgrid_delete(hashgrid_t* spc) {
	object_clear(&spc->nodes);
	object_clear(&spc->pairs);
	object_clear(&spc->objects);
	hashmap_free(&spc->bodygrid);
	hashmap_free(&spc->objectmap);
}

void _hashgrid_pair(hashgrid_t* spc, element_t* A, element_t* B) {
	pair_t* pair = hashmap_get(&A->paired, B);
	if (!pair) {
		pair = object_alloc(&spc->pairs, sizeof(pair_t));
		pair->colliding = 0;
		pair->rc = 0;
		pair->data = 0;
		hashmap_put(&A->paired, B, pair);
		hashmap_put(&B->paired, A, pair);
	}
	pair->rc++;
}

void _hashgrid_unpair(hashgrid_t* spc, element_t* A, element_t* B) {
	pair_t* pair = hashmap_get(&A->paired, B);
	if (pair) {
		pair->rc--;
		if (pair->rc == 0) {
			if (pair->colliding && spc->unpair)
				spc->unpair(pair->data, spc->ud);

			hashmap_remove(&A->paired, B);
			hashmap_remove(&B->paired, A);
			object_free(&spc->pairs, pair);
		}
	}
	else printf("Error: pair does not exist");
}

void _hashgrid_testcollisions(hashgrid_t* spc, element_t* e) {
	element_t* o;  pair_t* pair;
	hashmap_foreach(&e->paired, o, pair) {
		int colliding = aabb_overlap(e->aabb, o->aabb);
		if (colliding != pair->colliding) {
			if (colliding && spc->pair)
				pair->data = spc->pair(e->id, o->id, spc->ud);
			else if (!colliding && spc->unpair)
				spc->unpair(pair->data, spc->ud);

			pair->colliding = colliding;
		}
	}
}

void _hashgrid_bodyinsert(hashgrid_t* spc, element_t* e, rect2 aabb) {
	float tile_size = spc->tile_size;
	rect2 tilerect = { aabb[0] / tile_size, aabb[1] / tile_size, aabb[2] / tile_size, aabb[3] / tile_size };
	for (int i = tilerect[0]; i <= tilerect[2]; i++) {
		for (int j = tilerect[1]; j <= tilerect[3]; j++) {
			uint32_t c = mortonencode(i, j);
			//Insert body into hashgrid
			gridnode_t* node = hashmap_get(&spc->bodygrid, c);
			if (!node) {
				node = object_alloc(&spc->nodes, sizeof(gridnode_t));
				hashmap_init(node);
				hashmap_put(&spc->bodygrid, c, node);
			}
			int inserted = hashmap_get(node, e);
			if (inserted == 0) {
				element_t* o; int rc;
				hashmap_foreach(node, o, rc) {
					if (o->id == e->id)
						continue;
					_hashgrid_pair(spc, e, o);
				}
			}
			hashmap_put(node, e, ++inserted);
		}
	}
}

void _hashgrid_bodyremove(hashgrid_t* spc, element_t* e, rect2 aabb) {
	float tile_size = spc->tile_size;
	rect2 tilerect = { aabb[0] / tile_size, aabb[1] / tile_size, aabb[2] / tile_size, aabb[3] / tile_size };
	for (int i = tilerect[0]; i <= tilerect[2]; i++) {
		for (int j = tilerect[1]; j <= tilerect[3]; j++) {
			uint32_t c = mortonencode(i, j);
			//Remove body from hashgrid
			gridnode_t* node = hashmap_get(&spc->bodygrid, c);
			int inserted = hashmap_get(node, e);
			if (inserted == 1) {
				hashmap_remove(node, e);
				if (hashmap_isempty(node)) {
					hashmap_free(node);
					hashmap_remove(&spc->bodygrid, c);
					object_free(&spc->nodes, node);
				}
				else {
					element_t* o;  int rc;
					hashmap_foreach(node, o, rc) {
						if (o->id == e->id)
							continue;
						_hashgrid_unpair(spc, e, o);
					}
				}
			}
			else
				hashmap_put(node, e, --inserted);
		}
	}
}


void hashgrid_create(hashgrid_t* spc, UID id) {
	element_t* e = hashmap_get(&spc->objectmap, id);
	if (!e) {
		e = object_alloc(&spc->objects, sizeof(element_t));
		hashmap_init(&e->paired);
		aabb_zero(e->aabb);
		e->id = id;
		e->pass = 0;
		hashmap_put(&spc->objectmap, id, e);
	}
}


void hashgrid_remove(hashgrid_t* spc, UID id) {
	element_t* e = hashmap_get(&spc->objectmap, id);
	if (e) {
		_hashgrid_bodyremove(spc, e, e->aabb);
		hashmap_free(&e->paired);
		object_free(&spc->objects, e);
		hashmap_remove(&spc->objectmap, id);
	}
}


void hashgrid_move(hashgrid_t* spc, UID id, rect2 aabb, bool collide) {
	element_t* e = hashmap_get(&spc->objectmap, id);
	assert(e);

	if (memcmp(aabb, e->aabb, sizeof(rect2)) == 0)
		return;
	if (memcmp(aabb, &NULL_RECT, sizeof(rect2)) != 0)
		_hashgrid_bodyinsert(spc, e, aabb);
	if (memcmp(e->aabb, &NULL_RECT, sizeof(rect2)) != 0)
		_hashgrid_bodyremove(spc, e, e->aabb);

	memcpy(e->aabb, aabb, sizeof(rect2));

	//Can produce new Collision instances
	if (collide)
		_hashgrid_testcollisions(spc, e);

	//memcpy(body->aabb, aabb, sizeof(rect2));
}


int hashgrid_cullray(hashgrid_t* spc, vec2 begin, vec2 end, UID* results, int max_results) {
	spc->pass++;
	int count = 0;
	float tile_size = spc->tile_size;
	vec2 step = { end[0] - begin[0], end[1] = begin[1] };
	vec2_normalize(step);
	vec2 d = { tile_size / fabs(step[0]), tile_size / fabs(step[1]) };
	vec2 pos = { floor(begin[0] / tile_size), floor(begin[1] / tile_size) };
	vec2 to = { floor(end[0] / tile_size), floor(end[1] / tile_size) };
	vec2 max = {
		(floor(pos[0] + (step[0] < 0 ? 0 : 1)) * tile_size - begin[0]) / step[0],
		(floor(pos[1] + (step[1] < 0 ? 0 : 1)) * tile_size - begin[1]) / step[1] };
	step[0] = SGN(step[0]);
	step[1] = SGN(step[1]);
	bool reached_x = false;
	bool reached_y = false;
	while (true) {
		if (max[0] < max[1]) {
			max[0] += d[0];
			pos[0] += step[0];
		}
		else {
			max[1] += d[1];
			pos[1] += step[1];
		}

		if (step[0] > 0)
			if (pos[0] >= end[0])
				reached_x = true;
			else if (pos[0] <= end[0])
				reached_x = true;

		if (step[1] > 0)
			if (pos[1] >= end[1])
				reached_y = true;
			else if (pos[1] <= end[1])
				reached_y = true;

		gridnode_t* node = hashmap_get(&spc->bodygrid, mortonencode(pos[0], pos[1]));
		if (node) {
			element_t* e; int _;
			hashmap_foreach(node, e, _) {
				if (count >= max_results)
					break;
				if (spc->pass == e->pass)
					continue;

				if (aabb_segment(e->aabb, begin, end, NULL, NULL)) {
					e->pass = spc->pass;
					results[count++] = e->id;
				}
			}
		}
		if (reached_x && reached_y)
			break;
	}
	return count;
}


int hashgrid_cullaabb(hashgrid_t* spc, rect2 aabb, UID* results, int max_results) {
	spc->pass++;
	int count = 0;
	float tile_size = spc->tile_size;
	rect2 tilerect = { aabb[0] / tile_size, aabb[1] / tile_size, aabb[2] / tile_size, aabb[3] / tile_size };
	aabb_correct(tilerect);
	for (int j = tilerect[1]; j <= tilerect[3]; j++) {
		for (int i = tilerect[0]; i <= tilerect[2]; i++) {
			gridnode_t* node = hashmap_get(&spc->bodygrid, mortonencode(i, j));
			if (node) {
				element_t* e; int _;
				hashmap_foreach(node, e, _) {
					if (count >= max_results)
						break;
					if (spc->pass == e->pass)
						continue;

					e->pass = spc->pass;
					if (aabb_overlap(aabb, e->aabb))
						results[count++] = e->id;
				}
			}
		}
	}
	return count;
}
