#pragma once
#include "data.h"
#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Event_mt "event"

#define event_getdelegates(_handler, _evt) \
	hashmap_get(&(_handler)->delegate_map, _evt)

#define EVENT_TYPES \
	X(none) \
	X(quit) \
	X(press) \
	X(release) \
	X(text) \
	X(fileread) \
	X(filewrite) \
	X(preupdate) \
	X(update) \
	X(postupdate) \
	X(predraw) \
	X(draw) \
	X(postdraw) \
	X(pregui) \
	X(gui) \
	X(postgui) \
	X(addcomponent) \
	X(setcomponent) \
	X(delcomponent) \
	X(addtype) \
	X(deltype) \
	X(startcollision) \
	X(endcollision)

enum event_enum {
#define X(_name) on_##_name,
	EVENT_TYPES
#undef X
	NUM_EVENTS
};

enum event_mask {
#define X(_name) _name = 1 << on_##_name,
	EVENT_TYPES
#undef X
};

typedef struct {
	uint32_t type;
	variant_t p0;
	variant_t p1;
} event_t;

typedef int(*evt_callback_t) (void*, event_t);

typedef struct {
	void* next;
	void* prev;
	union {
		struct {
			void* receiver;
			evt_callback_t callback;
		};
		struct {
			int lua_ref;
			int lua_cb;
		};
	};
	lua_State* L;
} delegate_t;

typedef struct {
	object_allocator_t;
	queue_t queue;
	hashmap_t delegate_map;
} eventhandler_t;

void eventhandler_init(eventhandler_t* eventhandler, const char* name);
void event_register(eventhandler_t* eventhandler, void* receiver, int evt, evt_callback_t cb, lua_State* L);
int event_post(eventhandler_t* eventhandler, event_t evt);
void event_dispatch(eventhandler_t* eventhandler, event_t evt);
void event_pump(eventhandler_t* eventhandler);

inline GLOBAL_SINGLETON(eventhandler_t, eventhandler, "EventHandler");

int openlib_Event(lua_State* L);
