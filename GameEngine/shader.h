#pragma once
#include <glad.h>
#include <lauxlib.h>

#define Shader_mt "Shader"

typedef struct Shader {
	unsigned int shd_ID;
	unsigned int stages;
} Shader;

void shader_compile(Shader* shader, const GLchar* source);
void shader_delete(Shader* shader);
void shader_use(Shader* shader);
Shader* shader_getcurrent();

static int shader_compile_error(GLuint obj);
static int program_compile_error(GLuint obj);

int openlib_Shader(lua_State* L);