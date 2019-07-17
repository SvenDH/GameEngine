#pragma once
#include "utils.h"
#include "data.h"
#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define EVENT_BUFFER_SIZE 4096
#define Event_mt "Event"

#define MAX_EVENT_DATA 1024

#define EVENT_TYPES \
	X(EVT_NONE, NONE) \
	X(EVT_QUIT, QUIT) \
	X(EVT_INPUT, INPUT) \
	X(EVT_TEXT, TEXT) \
	X(EVT_DRAW, DRAW) \
	X(EVT_UPDATE, UPDATE) \
	X(EVT_NEWENTITY, NEWENTITY) \
	X(EVT_DELENTITY, DELENTITY) \
	X(EVT_NEWWORLD, NEWWORLD)

#define X C_ENUM_HELPER
typedef enum event_t {
	EVENT_TYPES
	MAX_EVENTS
} event_t;
#undef X

typedef void(*event_cb) (void *, event_t, char *, size_t);
//cb is either callback or lua function, data can be lua_State*
typedef struct {
	event_cb cb;
	void* data;
	struct Listener* next;
	int use_lua;
} Listener;

typedef struct {
	RingBuffer* event_queue;
	MemoryPool* pool;
	Listener* listeners[MAX_EVENTS];
} EventManager;

Listener* event_register(event_t evt, event_cb cb, void* data);
int event_push(event_t evt, char* data, size_t size);
void event_pump();

int openlib_Event(lua_State* L);
