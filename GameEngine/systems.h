#pragma once
#include "data.h"
#include "ecs.h"
#include "component.h"
#include "event.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define MAX_SYSTEMS 256

typedef void(*chunk_iter_cb) (Chunk*);

typedef struct {
	int id;
	int mask;
	RingBuffer queue;
	int nr_worlds;
	int worlds[MAX_WORLDS];
} System;

System* system_init(event_t evt, event_cb cb, int component_mask);

void system_iter(System* system, chunk_iter_cb callback);

void debug_system(System* system, event_t evt, char* data, size_t len);
