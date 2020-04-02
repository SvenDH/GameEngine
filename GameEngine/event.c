#include "event.h"
#include "memory.h"
#include "data.h"
#include "utils.h"

#define EVENT_BUFFER_SIZE 4096

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

void eventhandler_init(eventhandler_t* eventhandler, const char* name) {
	assert(NUM_EVENTS < 32);
	object_init((object_allocator_t*)eventhandler, name, sizeof(delegate_t), 16, 4);
	queue_init(&eventhandler->queue, sizeof(event_t));
	hashmap_init(&eventhandler->delegate_map);
}

void event_register(void* receiver, int evt, evt_callback_t cb, lua_State* L) {
	eventhandler_t* handler = eventhandler_instance();
	for (int i = 0; i < NUM_EVENTS; i++) {
		if ((1 << i) & evt) {
			delegate_t* l = object_alloc((object_allocator_t*)handler, sizeof(delegate_t));
			delegate_t* list = event_getdelegates(handler, i);
			l->next = list;
			l->prev = NULL;
			l->callback = cb;
			l->receiver = receiver;
			l->L = L;
			if (list) 
				list->prev = l;
			hashmap_put(&handler->delegate_map, i, (void*)l);
		}
	}
}

//Immediately call the listeners for event
void event_post(event_t evt) {
	for (delegate_t* l = event_getdelegates(eventhandler_instance(), evt.type); l; l = l->next) {
		if (l->L) {
			lua_rawgeti(l->L, LUA_REGISTRYINDEX, l->lua_cb);
			lua_rawgeti(l->L, LUA_REGISTRYINDEX, l->lua_ref);
			lua_pushinteger(l->L, evt.type);
			lua_pushinteger(l->L, evt.p0.data);
			lua_pushinteger(l->L, evt.p1.data);
			lua_call(l->L, 4, 1);
			if (lua_toboolean(l->L, -1)) break;
		}
		else if (l->callback(l->receiver, evt)) break;
	}
}

//Put event on queue
int event_postlater(event_t evt) {
	return queue_put(&eventhandler_instance()->queue, &evt);
}

//Empty the event queue and dispatch events to all listeners
void event_pump() {
	event_t evt;
	queue_t* b = &eventhandler_instance()->queue;
	while (queue_get(b, &evt))
		event_post(evt);
}

//event_t.listen(obj, evt, func [, filter])
static int w_event_listen(lua_State *L) {
	lua_pushvalue(L, 1);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	int evt = luaL_checkinteger(L, 2);
	int cb = 0;
	if (lua_isfunction(L, 3)) {
		lua_pushvalue(L, 3);
		cb = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	event_register(ref, evt, cb, L);
	return 0;
}

//event_t(num, data)
static int w_event_new(lua_State *L) {
	double nr;
	event_t* evt = lua_newuserdata(L, sizeof(event_t));
	luaL_setmetatable(L, Event_mt);
	evt->type = luaL_checkinteger(L, 2);
	switch (lua_type(L, 3)) {
	case LUA_TNUMBER:
		nr = lua_tonumber(L, 3);
		if (nr == (int)nr) evt->p0.data = nr;
		else evt->p0.nr = nr;
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		evt->p0.data = 0;
		break;
	default:
		lua_pushvalue(L, 3);
		evt->p0.data = luaL_ref(L, LUA_REGISTRYINDEX);
		break;
	}
	return 1;
}

//event_t.dispatch(evt, [, data])
static int w_event_postnow(lua_State *L) {
	event_t* evt = luaL_checkudata(L, 1, Event_mt);
	event_post(*evt);
	return 0;
}

//event_t.post(evt [, data])
static int w_event_postlater(lua_State *L) {
	event_t* evt = luaL_checkudata(L, 1, Event_mt);
	event_postlater(*evt);
	return 0;
}

int openlib_Event(lua_State* L) {
	static luaL_Reg event_func[] = {
		{"post", w_event_postlater},
		{"dispatch", w_event_postnow},
		{"listen", w_event_listen},
		{NULL, NULL},
	};
	create_lua_class(L, Event_mt, w_event_new, event_func);

	lua_getglobal(L, Event_mt);
#define X(_name) \
	lua_pushstring(L, "on_" #_name); \
	lua_pushnumber(L, _name); \
	lua_settable(L, -3);
EVENT_TYPES
#undef X
	lua_setglobal(L, "event");
	return 0;
}