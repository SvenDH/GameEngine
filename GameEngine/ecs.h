#pragma once
#include "types.h"
#include "event.h"
#include "data.h"
#include "utils.h"
#include "resource.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Entity_mt		"entity"
#define Component_mt	"component"
#define Chunk_mt		"chunk"

#define TILE_ROWS 8
#define TILE_COLS 8

#define MAX_COMPONENTS sizeof(type_t) * 8

typedef uint64_t entity_t; //TODO: add index, user-id and salt
typedef uint32_t type_t;
typedef uint32_t chunk_t;

#define TAG_DECLARE(_name) typedef struct {char _no;} _name; int E_##_name

#define COMP_DECLARE(_name) _name; int E_##_name

#define COMP_TYPE(_name) (E_##_name)

#define COMP_FLAG(_x) (1<<(E_##_x))


/* builtin types and tags: */
typedef struct {
	entity_t id;
} COMP_DECLARE(EntityId);

typedef struct {
	chunk_t children;
} COMP_DECLARE(Container);

typedef struct {
	vec2 position; //TODO: make position component and rotation
} COMP_DECLARE(Transform);

TAG_DECLARE(Disabled);

#define COMP_ID (0)
#define FLAG_ID (1)

typedef struct {
	entity_t ent;
	type_t type;
} component_ref;

typedef enum  {
	ATT_NONE,
	ATT_BYTE,
	ATT_SHORT,
	ATT_INTEGER,
	ATT_FLOAT,
	ATT_VEC2,
	ATT_ENTITY,
	ATT_CHUNK,
	ATT_RESOURCE,
	ATT_COUNT
} component_attribute_types;

typedef struct {
	const char* name;
	component_attribute_types type;
	ushort offset;
} component_attribute_info;

typedef struct {
	const char* name;
	size_t size;
	const component_attribute_info* attribute;
} component_type_info;

#define COMPONENT_LAYOUT_END NULL, ATT_COUNT, 0

typedef enum {
	SYS_NONE,
	SYS_ADDCOMP,
	SYS_DELCOMP,
	SYS_SETCOMP,
	//From here on update systems that need archetype checking
	SYS_UPDATE,
	SYS_DRAW,
	NUM_SYSTYPES,
} system_type;

typedef struct {
	object_allocator_t chunks;
	object_allocator_t archetypes;
	object_allocator_t systems;

	hashmap_t entity_map;
	hashmap_t chunk_map;
	hashmap_t archetype_map;
	listnode_t system_lists[NUM_SYSTYPES];

	int chunk_index;
	int component_counter;
	component_type_info component_info[MAX_COMPONENTS];
} world_t;

/* Use macros to define components in the lua openLib functions with L */
#define TAG_DEFINE(_wld, _name, _type) \
E_##_type = ecs_addcomponenttype(_wld, _name, 0, NULL, L); \
lua_pushinteger(L, 1 << E_##_type); \
printf("%s: %i\n", _name, E_##_type); \
lua_setglobal(L, #_type)

#define COMP_DEFINE(_wld, _name, _type, ...) \
static component_attribute_info _type##layout[] = __VA_ARGS__; \
E_##_type = ecs_addcomponenttype(_wld, _name, sizeof(_type), _type##layout, L); \
lua_pushinteger(L, 1 << E_##_type); \
printf("%s: %i\n", _name, E_##_type); \
lua_setglobal(L, #_type)
/* Componentname = COMP_FLAG(Component) = 1 << COMP_TYPE(Component) */
//TODO: make component names objects with custom ops

int ecs_addcomponenttype(world_t* wld, const char* name, size_t size, component_attribute_info* layout, lua_State* L);

typedef struct {
	uint32_t _magic;			//4
	chunk_t id;					//8
	type_t type;				//12
	entity_t parent;			//20
	uint16_t ent_count;			//22
	uint16_t max_ents;			//24
	chunk_t next_children;		//28
	chunk_t prev_children;		//32
	chunk_t next_chunk;			//36
	chunk_t prev_chunk;			//40
	byte data[4];
} chunk_data;

typedef struct {
	type_t type;
	uint16_t max_ents;
	uint16_t chunk_count;
	chunk_t chunk_list;
} type_data;

typedef struct {
	lua_State* state;
	int chunk;
	const char* code;
} group_t;

typedef struct {
	world_t* wld;
	chunk_data* chunk;
	uint16_t start;
	uint16_t end;
	double delta_time;
	void* data;
} task_data;

typedef void(*system_task_t) (task_data*);

typedef struct {
	listnode_t;
	world_t* world;
	system_task_t cb;
	system_type type;
	group_t filter;
	hashmap_t archetypes;
	uint32_t pass;
	void* data;
} system_t;

#define SYSTEM_DEFINE(_wld, _data, _action, _type, ...) \
static const char* _action##code = "return (" #__VA_ARGS__ ")"; \
if (luaL_loadstring(L, _action##code) == LUA_OK) { \
	system_new((_wld), _action, (_type), (group_t){ .state = L, .chunk = luaL_ref(L, LUA_REGISTRYINDEX), .code = _action##code }, (_data)); \
} else { \
	printf("Error loading filter: %s \n", lua_tostring(L, -1)); \
	lua_pop(L, 1); \
}

system_t* system_new(world_t* wld, evt_callback_t cb, system_type type, group_t filter, void* data);
void system_delete(world_t* wld, system_t* system);
int system_typelistener(world_t* wld, event_t evt);
int system_updatelistener(world_t* wld, event_t evt);

#define SYSTEM_ENTRY(_name) \
void _name (task_data* task)

#define COMPARRAY_GET(_type, _name) \
_type* _name = (_type*)chunk_getcomponentarray(task->wld, task->chunk, COMP_TYPE(_type))

#define ENTITY_GETCOMP(_wld, _type, _name, _ent) \
_type* _name = (_type*)entity_getcomponent(_wld, _ent, COMP_TYPE(_type));

#define PUSH_COMPONENT(_ent, _comp) \
*(component_ref*)lua_newuserdata(L, sizeof(component_ref)) = (component_ref){ (_ent), (_comp) }; \
luaL_setmetatable(L, Component_mt)

void world_init(world_t* world, const char* name);

void ecs_addcomponents(world_t* wld, entity_t ent, type_t mask);
void ecs_delcomponents(world_t* wld, entity_t ent, type_t mask);
int ecs_debug_log(world_t* wld, event_t evt);

chunk_data* chunk_new(world_t* wld, type_t type, entity_t parent);
chunk_data* chunk_get(world_t* wld, chunk_t id);
chunk_data* chunk_find(world_t* wld, type_t type, entity_t parent, int create);
void chunk_link(world_t* wld, chunk_data* chunk);
void chunk_relink(world_t* wld, chunk_data* chunk);
int chunk_change(world_t* wld, chunk_data* chunk, type_t new_type, entity_t new_parent);
void chunk_delete(world_t* wld, chunk_data* chunk);
int chunk_componentcopy(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src);
int chunk_componentmove(world_t* wld, chunk_data* dest_chunk, chunk_data* src_chunk, int dest, int src);
void* chunk_getcomponentarray(world_t* wld, chunk_data* chunk, int component);
void* chunk_getcomponent(world_t* wld, chunk_data* chunk, int component, int index);
chunk_data* component_to_chunk(world_t* wld, void* comp_ptr, int comp_type, int* index);

void get_integer(lua_State* L, int32_t* integer);
void set_integer(lua_State* L, int32_t* integer, int i);
void get_short(lua_State* L, int16_t* integer);
void set_short(lua_State* L, int16_t* integer, int i);
void get_float(lua_State* L, float* num);
void set_float(lua_State* L, float* num, int i);
void get_vec2(lua_State* L, vec2* vector);
void set_vec2(lua_State* L, vec2* vector, int i);
void get_resource(lua_State* L, rid_t* resource);
void set_resource(lua_State* L, rid_t* resource, int i);
void get_entity(lua_State* L, entity_t* id);
void set_entity(lua_State* L, entity_t* id, int i);
void get_byte(lua_State* L, byte* array); void set_byte(lua_State* L, byte* array, int i);
void get_chunk(lua_State* L, chunk_t* child_index);

type_data* archetype_new(world_t* wld, type_t components);
type_data* archetype_get(world_t* wld, type_t type, int create);
void archetype_delete(world_t* wld, type_data* archetype);

entity_t entity_new(world_t* wld, entity_t id, type_t type, entity_t parent);
entity_t* entity_get(world_t* wld, entity_t id);
void* entity_getcomponent(world_t* wld, entity_t ent, int component);
int entity_move(world_t* wld, entity_t ent, chunk_data* dest_chunk, int dest);
int entity_change(world_t* wld, entity_t ent, entity_t new_parent, type_t new_type);
chunk_data* entity_to_chunk(world_t* wld, entity_t ent, int* index);
void entity_delete(world_t* wld, entity_t ent);

#define entity_gettype(_wld, _ent) \
(entity_to_chunk((_wld), (_ent), NULL)->type)

#define entity_getparent(_wld, _ent) \
entity_to_chunk((_wld), (_ent), NULL)->parent

#define entity_changetype(_wld, _ent, _new_type) \
entity_change((_wld), (_ent), entity_getparent(_wld, _ent), (_new_type))

#define entity_changeparent(_wld, _ent, _new_parent) \
entity_change((_wld), (_ent), (_new_parent), entity_gettype(_wld, _ent))

#define archetype_foreach(_wld, _arch, _chunk) \
	for (_chunk = (_arch)->chunk_list; \
		_chunk; _chunk = chunk_get((_wld), _chunk)->next_chunk)

#define children_foreach(_wld, _first, _chunk) \
	for (chunk_t _idc = (_first); \
		_idc, _chunk = chunk_get((_wld), _idc); \
		_idc = (_chunk)->next_children)

#define entity_hascomponent(_wld, _ent, _comp) \
	((entity_gettype(_wld, _ent) & COMP_FLAG(_comp)) != 0)

inline GLOBAL_SINGLETON(world_t, world, "World");

int openlib_ECS(lua_State* L);
