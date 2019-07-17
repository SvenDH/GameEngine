#include "event.h"

static EventManager manager;

int event_push(event_t evt, char* data, size_t size) {
	assert(size <= MAX_EVENT_DATA);
	char e = evt;
	if (manager.event_queue->max_ - manager.event_queue->size_ > size + 5) {
		serialize_bytes(manager.event_queue, &e, 1);
		serialize_string(manager.event_queue, data, &size);
		return 0;
	}
	return 1;
}

Listener* event_register(event_t evt, event_cb cb, void* data) {
	Listener* listener = (Listener*)pool_alloc(manager.pool);
	listener->cb = cb;
	listener->next = manager.listeners[evt];
	listener->data = data;
	listener->use_lua = 0;
	manager.listeners[evt] = listener;
	return listener;
}

//Empty the event queue and post events to all listeners
void event_pump() {
	char evt;
	char data[MAX_EVENT_DATA];
	size_t len;
	Listener* listener;

	while (manager.event_queue->size_) {
		deserialize_bytes(manager.event_queue, &evt, 1);
		deserialize_string(manager.event_queue, data, &len);
		//Check listener->use_lua to see if this is a lua-listener or C-listener
		for (listener = manager.listeners[evt]; listener; listener = listener->next) {
			if (listener->use_lua) {
				lua_rawgeti(listener->data, LUA_REGISTRYINDEX, listener->cb);
				if (lua_isfunction(listener->data, -1)) {
					lua_pushlstring(listener->data, data, len);
					lua_call(listener->data, 1, 0);
				}
			}
			else listener->cb(listener->data, evt, data, len);
		}
	}
}

//Event.listener(evt, func)
static int event_listen(lua_State *L) {
	event_t evt = luaL_checkinteger(L, 1);
	Listener* listener = event_register(evt, luaL_ref(L, LUA_REGISTRYINDEX), L);
	listener->use_lua = 1;
	return 0;
}

//Event.post(evt [, data])
static int event_post(lua_State *L) {
	size_t len;
	event_t evt = luaL_checkinteger(L, 1);
	char* data;
	double nr;
	switch (lua_type(L, 2)) {
	case LUA_TSTRING:
		data = lua_tolstring(L, 2, &len);
		event_push(evt, data, len);
		break;
	case LUA_TNUMBER:
		nr = lua_tonumber(L, 2);
		if (nr == (int)nr) event_push(evt, &nr, sizeof(int));
		else event_push(evt, &nr, sizeof(double));
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		event_push(evt, NULL, 0);
		break;
	}	
	return 0;
}

int openlib_Event(lua_State* L) {
	manager.event_queue = ringbuffer_init(EVENT_BUFFER_SIZE);
	manager.pool = pool_init(sizeof(Listener), 64);
	for (int i = 0; i < MAX_EVENTS; i++)
		manager.listeners[i] = NULL;

	static luaL_Reg event_func[] = {
		{"post", event_post},
		{"listen", event_listen},
		{NULL, NULL},
	};
	create_lua_lib(L, Event_mt, event_func);
	lua_newtable(L);
#define X LUA_ENUM_HELPER
	EVENT_TYPES
#undef X
	lua_setglobal(L, "EVENT");
	return 0;
}