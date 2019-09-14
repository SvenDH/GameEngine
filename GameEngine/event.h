#pragma once
#include "data.h"
#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define Event_mt "Event"

#define EVENT_TYPES \
	X(NONE) \
	X(QUIT) \
	X(INPUT) \
	X(TEXT) \
	X(FILEREAD) \
	X(FILEWRITE) \
	X(PREUPDATE) \
	X(UPDATE) \
	X(POSTUPDATE) \
	X(PREDRAW) \
	X(DRAW) \
	X(POSTDRAW) \
	X(PREGUI) \
	X(GUI) \
	X(POSTGUI) \
	X(NEWENTITY) \
	X(CHANGEENTITY) \
	X(DELENTITY) \
	X(NEWCOMPONENT) \
	X(DELCOMPONENT) \
	X(NEWTYPE) \
	X(DELTYPE)

typedef enum {
#define X(name) EVT_##name,
	EVENT_TYPES
#undef X
	NUM_EVENTS
};

typedef struct {
	uint32_t type;
	union {
		uint64_t data;
		double nr;
		void* ptr;
		int ref;
	};
} Event;

typedef int(*Callback) (void*, Event);

typedef struct {
	void* next;
	void* prev;
	union {
		struct {
			void* receiver;
			Callback callback;
		};
		struct {
			int lua_ref;
			int lua_cb;
		};
	};
	lua_State* L;
} Delegate;

typedef struct {
	ObjectAllocator;
	queue_t queue;
	hashmap_t delegate_map;
} EventHandler;

EventHandler* eventhandler_instance();
void eventhandler_init(EventHandler* eventhandler, const char* name);
Delegate* event_register(EventHandler* eventhandler, void* receiver, event_t evt, Callback cb);
Delegate* event_getdelegates(EventHandler* eventhandler, event_t evt);
int event_post(EventHandler* eventhandler, Event evt);
void event_dispatch(EventHandler* eventhandler, Event evt);
void event_pump(EventHandler* eventhandler);

int openlib_Event(lua_State* L);
