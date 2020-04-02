#include "ecs.h"

//TODO: initialize chunks from file
void world_init(world_t* world, const char* name) {
	object_init(&world->chunks, "Ecs.Chunks", CHUNK_SIZE, 16, CHUNK_SIZE);
	object_init(&world->archetypes, "Ecs.Archetypes", sizeof(type_data), 16, 16);
	object_init(&world->systems, "Ecs.Systems", sizeof(system_t), 16, 16); //TODO: remove this and make external systems register

	hashmap_init(&world->entity_map);
	hashmap_init(&world->chunk_map);
	hashmap_init(&world->archetype_map);
	for (int i = 0; i < NUM_SYSTYPES; i++)
		list_init(&world->system_lists[i]);

	world->chunk_index = 1;
	world->component_counter = 0;
	memset(&world->component_info, 0, sizeof(component_type_info) * MAX_COMPONENTS);

	event_register(world,
		addtype|deltype, 
		system_typelistener, NULL);

	event_register(world,
		addcomponent|setcomponent|delcomponent|update|draw, 
		system_updatelistener, NULL);

	//event_register(world, 
	//	addcomponent|setcomponent|delcomponent|addtype|deltype, 
	//	ecs_debug_log, NULL);

}

int ecs_addcomponenttype(world_t* wld, const char* name, size_t size, struct component_attribute_info* layout, lua_State* L) {
	assert(wld->component_counter < sizeof(type_t) * 8);
	static component_attribute_info default_layout[] = { {COMPONENT_LAYOUT_END} };
	int type_id = wld->component_counter++;
	//Replaces layout names with lua interned strings
	if (layout) {
		for (component_attribute_info* att = layout; att->type != ATT_COUNT; att++) {
			att->name = intern_luastring(L, att->name, strlen(att->name));
		}
	}
	wld->component_info[type_id] = (component_type_info){
		intern_luastring(L, name, strlen(name)),
		size,
		(layout) ? layout : default_layout,
	};
	return type_id;
}

void ecs_addcomponents(world_t* wld, entity_t ent, type_t mask) {
	for (int i = 0; i < wld->component_counter; i++)
		if ((1 << i) & mask)
			event_post((event_t) {.type = on_addcomponent, .p0.data = ent, .p1.data = i});
}

void ecs_delcomponents(world_t* wld, entity_t ent, type_t mask) {
	for (int i = 0; i < wld->component_counter; i++)
		if ((1 << i) & mask)
			event_post((event_t) {.type = on_delcomponent, .p0.data = ent, .p1.data = i});
}

void ecs_setcomponents(world_t* wld, entity_t ent, type_t mask) {
	for (int i = 0; i < wld->component_counter; i++)
		if ((1 << i) & mask)
			event_post((event_t) {.type = on_setcomponent, .p0.data = ent, .p1.data = i});
}

int ecs_component_listener(world_t* wld, event_t evt) {
	group_t mask;
	system_t* system;
	chunk_data* chk;
	entity_t ent = (entity_t)evt.p0.data;
	int comp = (int)evt.p1.data;
	switch (evt.type) {	
	case on_delcomponent:
		if (comp == COMP_TYPE(Container)) {
			children_foreach(
				wld, 
				((Container*)entity_getcomponent(
					wld, ent, COMP_TYPE(Container)))->children,
				chk) {
				chunk_change(wld, chk, chk->type, 0);
			}
		}
	}
	return 0;
}


int ecs_debug_log(world_t* wld, event_t evt) {
	switch (evt.type) {
	case on_addcomponent:
		log_info("Created component of type %i for entity %0llx", (type_t)evt.p1.data, (entity_t)evt.p0.data);
		break;
	case on_setcomponent:
		log_info("Changed component of type %i for entity %0llx", (type_t)evt.p1.data, (entity_t)evt.p0.data);
		break;
	case on_delcomponent:
		log_info("Deleted component of type %i for entity %0llx", (type_t)evt.p1.data, (entity_t)evt.p0.data);
		break;
	case on_addtype:
		log_info("Created type: %i", (type_t)evt.p0.data);
		break;
	case on_deltype:
		log_info("Deleted type: %i", (type_t)evt.p0.data);
		break;
	}
	return 0;
}


//Entity(entity / id / table [, parent])
static int w_entity_call(lua_State* L) {
	const char* str;
	world_t* wld = world_instance();
	entity_t ent = 0;
	entity_t* ref;
	type_t components = 0;
	entity_t parent = 0;
	if (lua_gettop(L) > 2) {
		entity_t* parent_ref = luaL_checkudata(L, 3, Entity_mt);
		if (parent_ref) parent = *parent_ref;
	}
	switch (lua_type(L, 2)) {
	case LUA_TTABLE:
		for (int i = 0; i < wld->component_counter; i++) {
			lua_pushstring(L, wld->component_info[i].name);
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1))
				components |= 1<<i;
			lua_pop(L, 1);
		}
		ent = entity_new(wld, 0, components, parent);
		if (ent) {
			int index = 0, comp_index = 0;
			chunk_data* chunk = entity_to_chunk(wld, ent, &index);
			entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;
			set_component_from_table(L, 2);
			return 1;
		}
		break;
	case LUA_TSTRING:
		str = lua_tostring(L, 2);
		entity_t ref = strtol(str, NULL, sizeof(entity_t)); //TODO: fix this
		ent = *entity_get(wld, ref);
		if (ent) {
			entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
			luaL_setmetatable(L, Entity_mt);
			*ref = ent;
			return 1;
		}
		break;
	case LUA_TUSERDATA:
		ref = (entity_t*)luaL_checkudata(L, 2, Entity_mt);
		//TODO: copy entity ref to new entity
		break;

	}
	lua_pushnil(L);
	return 1;
}

//Entity:__index("id" / "components" / "parent" / "type")
static int w_entity_index(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	entity_t ent = *ref;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		chunk_data* chunk = entity_to_chunk(wld, ent, &index);
		if (chunk) {
			type_t mask = chunk->type;
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				if (chunk->parent) {
					entity_t* refpar = lua_newuserdata(L, sizeof(entity_t));
					luaL_setmetatable(L, Entity_mt);
					*refpar = chunk->parent;
				}
				else lua_pushnil(L);
			} else
#define KEY "type"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				lua_pushinteger(L, mask);
			}
			else
				for (int i = 0; i < wld->component_counter; i++) {
					//Interned string compare
					if (key == wld->component_info[i].name) {
						if (!((1<<i) & mask)) {
							entity_changetype(wld, ent, mask | (1 << i));
							chunk = entity_to_chunk(wld, *ref, &index);
						}
						PUSH_COMPONENT(ent, i);
						return 1;
					}
				}
			luaL_getmetatable(L, Entity_mt);
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
			return 1;
		}
		else return luaL_error(L, "error entity does not exist");
	}
	else return luaL_error(L, "error entity cannot be indexed this way");
}

//Entity:__newindex("id" / "components")
static int w_entity_newindex(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	entity_t ent = *ref;
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char *key = lua_tolstring(L, 2, &len);
		int index;
		chunk_data* chunk = entity_to_chunk(wld, ent, &index);
		if (chunk) {
			type_t mask = chunk->type;
#define KEY "parent"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				entity_t new_parent = chunk->parent;
				if (lua_type(L, 3) == LUA_TNIL)
					new_parent = 0;
				else
					new_parent = *(entity_t*)luaL_checkudata(L, 3, Entity_mt);
				entity_changeparent(wld, ent, new_parent);
				return 0;
			} else
#define KEY "type"
			if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
				type_t new_type = (type_t)luaL_checkinteger(L, 3);
				entity_changetype(wld, ent, new_type);
				return 0;
			}
			else {
				for (int i = 0; i < wld->component_counter; i++) {
					//Interned string compare
					if (key == wld->component_info[i].name) {
						if (!((1 << i) & mask)) {
							entity_changetype(wld, ent, mask | (1 << i));
							chunk = entity_to_chunk(wld, ent, &index);
						}
						PUSH_COMPONENT(ent, i);
						set_component_from_table(L, 3);
						return 0;
					}
				}
			}
			return luaL_error(L, "error entity does not have this attribute: %s", key);
		}
		else return luaL_error(L, "error entity does not exist");
	}
	else return luaL_error(L, "error entity cannot be indexed this way");
}

//Entity:add_component()
static int w_entity_add(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | (1 << comp_id))) == 0);
	return 1;
}

//Entity:remove_component()
static int w_entity_remove(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	int comp_id = luaL_checkinteger(L, 2);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~(1 << comp_id))) == 0);
	return 1;
}

//Entity:enable()
static int w_entity_enable(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type & ~COMP_FLAG(Disabled))) == 0);
	return 1;
}

//Entity:disable()
static int w_entity_disable(lua_State* L) {
	world_t* wld = world_instance();
	entity_t* ref = (entity_t*)luaL_checkudata(L, 1, Entity_mt);
	lua_pushboolean(L, entity_changetype(wld, *ref, (entity_to_chunk(wld, *ref, NULL)->type | COMP_FLAG(Disabled))) == 0);
	return 1;
}

//Entity:__tostring()
static int w_entity_tostring(lua_State* L) {
	entity_t* ent = luaL_checkudata(L, 1, Entity_mt);
	char buff[17];
	sprintf(buff, "%0llx", *ent);
	lua_pushlstring(L, buff, 16);
	return 1;
}

//Entity:delete()
static int w_entity_gc(lua_State* L) {
	entity_t* ref = luaL_checkudata(L, 1, Entity_mt);
	entity_delete(world_instance(), *ref);
	return 0;
}

//Generate getters and setters for all components
int w_component_index(lua_State* L) {
	world_t* wld = world_instance();
	component_ref ref = *(component_ref*)luaL_checkudata(L, 1, Component_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		size_t len;
		const char* key = lua_tolstring(L, 2, &len);
		for (component_attribute_info* att = wld->component_info[ref.type].attribute;
			att->type != ATT_COUNT; att++) {
			if (key == att->name) { //Interned string compare
				void* comp = (char*)entity_getcomponent(wld, ref.ent, ref.type) + att->offset;
				switch (att->type) {
				case ATT_BYTE: get_byte(L, comp); break;
				case ATT_SHORT: get_short(L, comp); break;
				case ATT_INTEGER: get_integer(L, comp); break;
				case ATT_FLOAT: get_float(L, comp); break;
				case ATT_VEC2: get_vec2(L, comp); break;
				case ATT_ENTITY: get_entity(L, comp); break;
				case ATT_CHUNK: get_chunk(L, comp); break;
				case ATT_RESOURCE: set_resource(L, comp, 3); break;
				}
			}
		}
	}
	else lua_pushnil(L);
	return 1;
}

int w_component_newindex(lua_State* L) {
	world_t* wld = world_instance();
	component_ref ref = *(component_ref*)luaL_checkudata(L, 1, Component_mt);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
			size_t len;
		const char* key = lua_tolstring(L, 2, &len);
		for (component_attribute_info* att = wld->component_info[ref.type].attribute;
			att->type != ATT_COUNT; att++) {
			if (key == att->name) { //Interned string compare
				char* comp = (char*)entity_getcomponent(wld, ref.ent, ref.type) + att->offset;
				switch (att->type) {
				case ATT_BYTE: set_byte(L, comp, 3); break;
				case ATT_SHORT: set_short(L, comp, 3); break;
				case ATT_INTEGER: set_integer(L, comp, 3); break;
				case ATT_FLOAT: set_float(L, comp, 3); break;
				case ATT_VEC2: set_vec2(L, comp, 3); break;
				case ATT_ENTITY: set_entity(L, comp, 3); break;
				case ATT_CHUNK: set_chunk(L, comp, 3); break;
				case ATT_RESOURCE: set_resource(L, comp, 3); break;
				}
			}
		}
		event_post((event_t) {.type = on_setcomponent, .p0.data = ref.ent, .p1.data = ref.type});
	}
	return 0;
}

static int w_chunk_entities_iter(lua_State* L) {
	world_t* wld = world_instance();
	chunk_data* chk = *(chunk_data**)lua_touserdata(L, lua_upvalueindex(1));
	uint16_t index = (uint16_t)lua_tointeger(L, lua_upvalueindex(2));
	if (index < chk->ent_count) {
		entity_t* ent = (entity_t*)chunk_getcomponent(wld, chk, COMP_ID, index);
		entity_t* ref = (entity_t*)lua_newuserdata(L, sizeof(entity_t));
		luaL_setmetatable(L, Entity_mt);
		*ref = *ent;
		lua_pushnumber(L, ++index);
		lua_replace(L, lua_upvalueindex(2));
		return 1;
	}
	return 0;
}

static int w_chunk_index(lua_State* L) {
	chunk_data* chk = luaL_checkudata(L, 1, Chunk_mt);
	size_t len;
	const char *key;
	switch (lua_type(L, 2)) {
	case LUA_TSTRING:
		key = lua_tolstring(L, 2, &len);
#define KEY "entities"
		if (len == sizeof(KEY) - 1 && !memcmp(key, KEY, sizeof(KEY) - 1)) {
			lua_pushvalue(L, 1);
			lua_pushinteger(L, 0);
			lua_pushcclosure(L, w_chunk_entities_iter, 2);
		}
		else
#undef KEY
			lua_pushnil(L);
		break;
	}
	return 1;
}

static int w_chunk_gc(lua_State* L) {
	chunk_data* chk = luaL_checkudata(L, 1, Chunk_mt);
	chunk_delete(world_instance(), chk);
	return 0;
}

int openlib_ECS(lua_State* L) {
	world_t* wld = world_instance();
	//Define entity id
	COMP_DEFINE(wld, "entity", EntityId, {
		{"id", ATT_ENTITY, offsetof(EntityId, id)},
		{COMPONENT_LAYOUT_END}
	});
	assert(COMP_TYPE(EntityId) == COMP_ID);
	//Define container for children
	COMP_DEFINE(wld, "container", Container, {
		{"children", ATT_CHUNK, offsetof(Container, children)},
		{COMPONENT_LAYOUT_END}
	});
	//Define transform
	COMP_DEFINE(wld, "transform", Transform, {
		{"position", ATT_VEC2, offsetof(Transform, position)},
		{COMPONENT_LAYOUT_END}
	});
	//Define disabled tag
	TAG_DEFINE(wld, "disabled", Disabled);

	static luaL_Reg ent_func[] = {
		{"delete", w_entity_gc},
		{"enable", w_entity_enable},
		{"disable", w_entity_disable},
		{"add_component", w_entity_add},
		{"remove_component", w_entity_remove},
		{"__index", w_entity_index},
		{"__newindex", w_entity_newindex},
		{"__tostring", w_entity_tostring},
		{NULL, NULL}
	};
	create_lua_class(L, Entity_mt, w_entity_call, ent_func);

	static luaL_Reg comp_func[] = {
		{"__newindex", w_component_newindex},
		{"__index", w_component_index},
		{NULL, NULL}
	};
	create_lua_type(L, Component_mt, comp_func);

	static luaL_Reg chunk_func[] = {
		{"delete", w_chunk_gc},
		{"__index", w_chunk_index},
		{NULL, NULL}
	};
	create_lua_type(L, Chunk_mt, chunk_func);
	
	return 0;
}
