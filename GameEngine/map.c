#include "data.h"
#include "utils.h"

#define HASHMAP_MAX_CHAIN_LENGTH 8
#define INITIAL_MAP_SIZE	4 * sizeof(hash_node)

#define MAP_SIZE(_map)	((_map)->cap / sizeof(hash_node))
#define MAP_ENTRY(_map, _i) (((hash_node*)m->data)[(_i)])
#define MAP_MASK(_map)	(MAP_SIZE(_map) - 1)

void hashmap_init(hashmap_t* m) {
	ALLOCATOR_INIT(m, calloc(INITIAL_MAP_SIZE, sizeof(hash_node)), INITIAL_MAP_SIZE);
}

int hashmap_hash(hashmap_t* m, key_t key) {
	const int mask = MAP_MASK(m);
	const hash = hash_int(key);
	int curr = hash & mask;
	for (int i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {
		if (MAP_ENTRY(m, curr).count == 0)
			return curr;

		if (MAP_ENTRY(m, curr).count > 0 && (MAP_ENTRY(m, curr).key == key))
			return curr;

		curr = (curr + 1) & mask;
	}

	return -1;
}

int hashmap_rehash(hashmap_t* m) {
	hash_node* temp = (hash_node *)calloc(2 * MAP_SIZE(m), sizeof(hash_node));
	if (!temp) return -1;

	hash_node* curr = m->data;
	m->data = temp;

	int old_size = MAP_SIZE(m);
	m->cap = 2 * m->cap;
	m->used = 0;

	int status;
	for (int i = 0; i < old_size; i++) {
		if (curr[i].count == 0) continue;

		status = _hashmap_put(m, curr[i].key, curr[i].data);
		if (status != 0) return status;
	}
	free(curr);

	return 0;
}

int _hashmap_put(hashmap_t* m, key_t key, void* value) {
	int index = hashmap_hash(m, key);
	while (index < 0) {
		if (hashmap_rehash(m) == -1)
			return -1;
		
		index = hashmap_hash(m, key);
	}
	if (MAP_ENTRY(m, index).count == 0) 
		m->used += sizeof(hash_node);
	MAP_ENTRY(m, index) = (hash_node) { key, 1, value };

	return 0;
}

void* _hashmap_get(hashmap_t* m, key_t key) {
	int curr = hash_int(key) & MAP_MASK(m);

	for (int i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {
		if (MAP_ENTRY(m, curr).count > 0) {
			if (MAP_ENTRY(m, curr).key == key) {
				return MAP_ENTRY(m, curr).data;
			}
		}
		curr = (curr + 1) & MAP_MASK(m);
	}

	return NULL;
}

int _hashmap_remove(hashmap_t* m, key_t key) {
	int curr = hash_int(key) & MAP_MASK(m);
	for (int i = 0; i < HASHMAP_MAX_CHAIN_LENGTH; i++) {
		if (MAP_ENTRY(m, curr).count > 0) {
			if (MAP_ENTRY(m, curr).key == key) {
				MAP_ENTRY(m, curr) = (hash_node){NULL, 0, NULL};
				m->used-= sizeof(hash_node);
				return 1;
			}
		}
		curr = (curr + 1) & MAP_MASK(m);
	}
	return 0;
}

void hashmap_free(hashmap_t* m) {
	free(m->data);
}
