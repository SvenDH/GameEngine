#pragma once
#include "utils.h"
#include "data.h"

typedef struct Entity {
	UID id;
} Entity;

Entity* entity_new(int world, int x, int y);
Entity* entity_get(UID* id);
void entity_delete(Entity* ent);
