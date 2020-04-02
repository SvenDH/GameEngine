#include "physics.h"
#include "event.h"
#include "utils.h"
#include "math.h"
#include "ecs.h"

void physics_init(physics_t* phs, const char* name) {
	//TODO: allocate spaces from allocator instead of resource allocator
	//object_init(&spaces, "spaces", sizeof(space_t), 4, 4);

	phs->gravity[0] = 0;
	phs->gravity[1] = 10 * DEFAULT_TILE_SIZE * 5;

	space_new(&phs->default_space, DEFAULT_TILE_SIZE, default_tilemap);
}

//Update pair parameters

//Calculate impulses for number of iterations

//Move bodies and recalculate speed

//Correct positions when penetrating

entity_t physics_onstartcollide(entity_t body, entity_t other) {
	world_t* wld = world_instance();
	int bodyhas = entity_hascomponent(wld, body, Body);
	int otherhas = entity_hascomponent(wld, other, Body);
	if (bodyhas || otherhas) {
		entity_t colid = entity_new(wld, 0, COMP_FLAG(Collision), 0);
		ENTITY_GETCOMP(wld, Collision, col, colid);
		if (bodyhas) {
			col->A = body;
			col->B = other;
		}
		else {
			col->A = other;
			col->B = body;
		}
		event_post((event_t) { .type = on_setcomponent, .p0.data = colid, .p1.data = COMP_TYPE(Collision) });
		return colid;
	}
	return 0;
}

void physics_onstopcollide(entity_t collision) {
	if (collision) {
		world_t* wld = world_instance();
		entity_delete(wld, collision);
	}
}

int openlib_Physics(lua_State* L) {
	world_t* wld = world_instance();
	//Define tile component
	COMP_DEFINE(wld, "tiles", Tiles, {
		{"data", ATT_BYTE, offsetof(Tiles, data)},
		{COMPONENT_LAYOUT_END}
	});

	//Define physics compnent
	COMP_DEFINE(wld, "body", Body, {
		{"speed", ATT_VEC2, offsetof(Body, speed)},
		{"force", ATT_VEC2, offsetof(Body, force)},
		{"mass",  ATT_FLOAT, offsetof(Body, mass)},
		{"friction", ATT_FLOAT, offsetof(Body, friction)},
		{"bounce", ATT_FLOAT, offsetof(Body, bounce)},
		{COMPONENT_LAYOUT_END}
	});

	//Define collider component
	COMP_DEFINE(wld, "collider", Collider, {
		{"offset", ATT_VEC2, offsetof(Collider, offset)},
		{"size", ATT_VEC2, offsetof(Collider, size)},
		{"layer", ATT_INTEGER, offsetof(Collider, layer)},
		{"mask", ATT_INTEGER, offsetof(Collider, mask)},
		{COMPONENT_LAYOUT_END}
	});

	//Define collision component
	COMP_DEFINE(wld, "collision", Collision, {
		{COMPONENT_LAYOUT_END}
	});

	physics_t* phs = physics_instance();

	SYSTEM_DEFINE(wld, phs, body_new, SYS_ADDCOMP, x & Body);
	SYSTEM_DEFINE(wld, phs, body_set, SYS_SETCOMP, x & Body);
	SYSTEM_DEFINE(wld, &phs->default_space, collider_new, SYS_ADDCOMP, x & Collider);
	SYSTEM_DEFINE(wld, &phs->default_space, collider_delete, SYS_DELCOMP, x & Collider);


	SYSTEM_DEFINE(wld, &phs->default_space, collider_update, SYS_UPDATE, Transform | Collider == x & (Transform | Collider));

	SYSTEM_DEFINE(wld, phs, body_integrateforces, SYS_UPDATE, x & Body);
	SYSTEM_DEFINE(wld, phs, contraint_update, SYS_UPDATE, x & Collision);
	SYSTEM_DEFINE(wld, phs, contraint_solve, SYS_UPDATE, x & Collision);
	SYSTEM_DEFINE(wld, phs, body_integratespeed, SYS_UPDATE, Body|Transform == x & (Body|Transform));
	//SYSTEM_DEFINE(wld, phs, contraint_correction, SYS_UPDATE, x & Collision);
	
	static luaL_Reg phs_lib[] = {
		{NULL, NULL}
	};
	create_lua_lib(L, Physics_mt, phs_lib);
	return 0;
}
