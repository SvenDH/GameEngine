#include "event.h"
#include "memory.h"
#include "data.h"
#include "utils.h"

#define EVENT_BUFFER_SIZE 4096

EventHandler* eventhandler_instance() {
	static EventHandler* eventhandler = NULL;
	if (!eventhandler) {
		eventhandler = (EventHandler*)malloc(sizeof(EventHandler));
		eventhandler_init(eventhandler, "EventHandler");
	}
	return eventhandler;
}

void eventhandler_init(EventHandler* eventhandler, const char* name) {
	objectallocator_init(eventhandler, name, sizeof(Delegate), 16, 4);
	queue_init(&eventhandler->queue, sizeof(Event));
	hashmap_init(&eventhandler->delegate_map);
}

Delegate* event_getdelegates(EventHandler* eventhandler, event_t evt) {
	Delegate* first = hashmap_get(&eventhandler->delegate_map, evt);
	return first;
}

Delegate* event_register(EventHandler* handler, void* receiver, event_t evt, Callback cb) {
	Delegate* l = objectallocator_alloc(handler, sizeof(Delegate));
	Delegate* list = event_getdelegates(handler, evt);

	l->next = list;
	l->prev = NULL;
	l->callback = cb;
	l->receiver = receiver;
	l->L = NULL;

	if (list) list->prev = l;
	hashmap_put(&handler->delegate_map, evt, (void*)l);

	return l;
}

//Immediately call the listeners for event
void event_dispatch(EventHandler* eventhandler, Event evt) {
	for (Delegate* l = event_getdelegates(eventhandler, evt.type); l; l = l->next) {
		if (l->L) {
			lua_rawgeti(l->L, LUA_REGISTRYINDEX, l->lua_cb);
			lua_rawgeti(l->L, LUA_REGISTRYINDEX, l->lua_ref);
			lua_pushinteger(l->L, evt.type);
			lua_pushinteger(l->L, evt.data);
			lua_call(l->L, 3, 1);
			if (lua_toboolean(l->L, -1)) break;
		}
		else if (l->callback(l->receiver, evt)) break;
	}
}

//Put event on queue
int event_post(EventHandler* eventhandler, Event evt) {
	return queue_put(&eventhandler->queue, &evt);
}

//Empty the event queue and dispatch events to all listeners
void event_pump(EventHandler* handler) {
	Event evt;
	queue_t* b = &handler->queue;
	while (queue_get(b, &evt))
		event_dispatch(handler, evt);
}

//Event.listen(obj, evt, func [, filter])
static int w_event_listen(lua_State *L) {
	lua_pushvalue(L, 1);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	event_t evt = luaL_checkinteger(L, 2);
	int cb = 0;
	if (lua_isfunction(L, 3)) {
		lua_pushvalue(L, 3);
		cb = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	Delegate* l = event_register(eventhandler_instance(), ref, evt, cb);
	l->L = L;
	return 0;
}

//Event(num, data)
static int w_event_new(lua_State *L) {
	double nr;
	Event* evt = lua_newuserdata(L, sizeof(Event));
	luaL_setmetatable(L, Event_mt);
	evt->type = luaL_checkinteger(L, 2);
	switch (lua_type(L, 3)) {
	case LUA_TNUMBER:
		nr = lua_tonumber(L, 3);
		if (nr == (int)nr) evt->data = nr;
		else evt->nr = nr;
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		evt->data = 0;
		break;
	default:
		lua_pushvalue(L, 3);
		evt->ref = luaL_ref(L, LUA_REGISTRYINDEX);
		break;
	}
	return 1;
}

//Event.dispatch(evt, [, data])
static int w_event_postnow(lua_State *L) {
	Event* evt = luaL_checkudata(L, 1, Event_mt);
	event_dispatch(eventhandler_instance(), *evt);
	return 0;
}

//Event.post(evt [, data])
static int w_event_postlater(lua_State *L) {
	Event* evt = luaL_checkudata(L, 1, Event_mt);
	event_post(eventhandler_instance(), *evt);
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

	lua_newtable(L);
#define X(name) \
	lua_pushstring(L, #name); \
	lua_pushnumber(L, EVT_##name); \
	lua_settable(L, -3);
EVENT_TYPES
#undef X
	lua_setglobal(L, "EVENT");
	return 0;
}