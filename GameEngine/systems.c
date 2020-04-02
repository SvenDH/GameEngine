#include "ecs.h"
#include "graphics.h"
#include "physics.h"


system_t* system_new(world_t* wld, evt_callback_t cb, system_type type, group_t filter, void* data) {
	system_t* system = object_alloc(&wld->systems, sizeof(system_t));
	list_add(&wld->system_lists[type], system);
	system->filter = filter;
	system->world = wld;
	system->cb = cb;
	system->type = type;
	system->data = data;
	system->pass = 0;
	hashmap_init(&system->archetypes);

	return system;
}

void system_delete(world_t* wld, system_t* system) {
	hashmap_free(&system->archetypes);
	list_remove(&wld->system_lists[system->type], system);
	object_free(&wld->systems, system);
}

bool system_applyfilter(system_t* system, type_t type) {
	//TODO: make L a thread or fix x
	lua_State* L = system->filter.state;
	lua_pushinteger(L, type);
	lua_setglobal(L, "x");
	lua_rawgeti(L, LUA_REGISTRYINDEX, system->filter.chunk);
	lua_call(L, 0, 1);
	int r;
	if (lua_type(L, -1) == LUA_TBOOLEAN)
		r = lua_toboolean(L, -1);
	else {
		r = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	return r != 0;
}

int system_typelistener(world_t* wld, event_t evt) {
	type_t type = evt.p0.data;
	system_t* system;
	switch (evt.type) {
	case on_addtype:
		for (int i = SYS_UPDATE; i < NUM_SYSTYPES; i++) {
			type_data* archetype = archetype_get(wld, type, 0);
			list_foreach(&wld->system_lists[i], system) {
				if (system_applyfilter(system, type)) {
					hashmap_put(&system->archetypes, archetype->type, archetype);
				}
			}
		}
		break;
	case on_deltype:
		for (int i = SYS_UPDATE; i < NUM_SYSTYPES; i++) {
			list_foreach(&wld->system_lists[i], system) {
				if (system_applyfilter(system, type)) {
					hashmap_remove(&system->archetypes, type);
				}
			}
		}
		break;
	}
	return 0;
}

int system_updatelistener(world_t* wld, event_t evt) {
	task_data task = {
		.wld = wld,
		.chunk = NULL,
		.start = 0,
		.end = 0,
		.delta_time = evt.p0.nr,
		.data = NULL };

	system_type type;
	switch (evt.type) {
	case on_addcomponent:	type = SYS_ADDCOMP; break;
	case on_setcomponent:	type = SYS_SETCOMP; break;
	case on_delcomponent:	type = SYS_DELCOMP; break;
	case on_update:			type = SYS_UPDATE;	break;
	case on_draw:			type = SYS_DRAW;	break;
	default:				type = SYS_NONE;	break;
	}
	system_t* system;
	if (type == SYS_DRAW || type == SYS_UPDATE) {
		list_foreach(&wld->system_lists[type], system) {
			//Update systems
			task.data = system->data;
			type_data* arch; chunk_t chunk; type_t ent_type;
			hashmap_foreach(&system->archetypes, ent_type, arch) {
				archetype_foreach(wld, arch, chunk) {
					task.chunk = chunk_get(wld, chunk);
					task.end = task.chunk->ent_count;
					system->cb(&task);
					system->pass++;
				}
			}
		}
	}
	else {
		type_t ent_type = 1 << evt.p1.data;
		list_foreach(&wld->system_lists[type], system) {
			if (system_applyfilter(system, ent_type)) {
				//Reactive systems
				task.data = system->data;
				int index;
				task.chunk = entity_to_chunk(wld, evt.p0.data, &index);
				task.start = index;
				task.end = index + 1;
				system->cb(&task);
				system->pass++;
			}
		}
	}
	return 0;
}
