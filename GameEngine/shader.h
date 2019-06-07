#pragma once
#include <glad/glad.h>
#include <lauxlib.h>

#define Shader_mt "Shader"

typedef struct Shader {
	unsigned int shd_ID;
	unsigned int stages;
} Shader;

void shader_bind(Shader* shader);

Shader* shader_getcurrent();

int openlib_Shader(lua_State* L);