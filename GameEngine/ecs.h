#pragma once
#include "types.h"
#include "event.h"
#include "component.h"
#include "data.h"
#include "utils.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Entity_mt		"entity"

#define entity_gettype(_wld, _ent) \
	entity_to_chunk((_wld), (_ent), NULL)->type

#define entity_getparent(_wld, _ent) \
	entity_to_chunk((_wld), (_ent), NULL)->parent

#define entity_changetype(_wld, _ent, _new_type) \
	entity_change((_wld), (_ent), entity_getparent(_wld, _ent), (_new_type))

#define entity_changeparent(_wld, _ent, _new_parent) \
	entity_change((_wld), (_ent), (_new_parent), entity_gettype(_wld, _ent))


typedef uint64_t entity_t; //TODO: add index, user-id and salt
typedef uint32_t type_t;

typedef struct {
	ObjectAllocator chunks;
	ObjectAllocator archetypes;
	ObjectAllocator systems;

	hashmap_t entity_map;
	hashmap_t chunk_map;
	hashmap_t archetype_map;

	int chunk_index;
} World;

typedef struct {
	uint32_t id;				//4
//	uint32_t _magic;
	type_t type;				//8
	entity_t parent;			//16
	uint32_t ent_count;			//20
	uint32_t max_ents;			//24
	uint32_t next_children;		//28
	uint32_t prev_children;		//32
	uint8_t sizes[MAX_COMPS];	//64	TODO: make this global
	char data[4];
} Chunk;

typedef struct {
	type_t type;
	int chunk_count;
	int max_ents;
	uint8_t sizes[MAX_COMPS];
	uint16_t offsets[MAX_COMPS];
	hashmap_t chunks;
} Archetype;

typedef struct {
	type_t and_mask;
	type_t not_mask;
	World* world;
	hashmap_t archetypes;
	void* data;
} System;

World* world_instance();

Chunk* chunk_new(World* wld, type_t type, entity_t parent);
Chunk* chunk_get(World* wld, type_t type, entity_t parent, int create);
void chunk_delete(World* wld, Chunk* chunk);
void* chunk_get_componentarray(Chunk* chunk, int component);
void* chunk_get_component(Chunk* chunk, int component, int index);

Archetype* archetype_new(World* wld, type_t components);
Archetype* archetype_get(World* wld, type_t type, int create);
void archetype_delete(World* wld, Archetype* archetype);

entity_t entity_new(World* wld, entity_t id, type_t type, entity_t parent);
entity_t* entity_get(World* wld, entity_t id);
void* entity_getcomponent(World* wld, entity_t ent, int component);
int entity_move(World* wld, Chunk* dest_chunk, Chunk* src_chunk, int dest, int src);
int entity_change(World* wld, entity_t ent, entity_t new_parent, type_t new_type);
Chunk* entity_to_chunk(World* wld, entity_t ent, int* index);
void entity_delete(World* wld, entity_t ent);
int entity_copy(Chunk* dest_chunk, Chunk* src_chunk, int dest, int src);

System* system_new(World* wld, Callback cb, type_t and_mask, type_t not_mask, event_t evt);
void system_delete(World* wld, System* system);

//int debug_system(System* system, Event evt);
int draw_system(System* system, Event evt);
int move_system(System* system, Event evt);

int openlib_ECS(lua_State* L);