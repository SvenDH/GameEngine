#include "graphics.h"
#include "utils.h"

static GLuint current = 0;

static int shader_compile_error(GLuint obj);
static int program_compile_error(GLuint obj);

int shader_compile(shader_t* shader, const char* source) {
	GLuint sVertex, sFragment;

	int success = 0;

	char *version = "#version 440\n";
	char *vshader[3] = { version, "#define VERTEX_PROGRAM\n", source };
	char *fshader[3] = { version, "#define FRAGMENT_PROGRAM\n", source };
	
	// Vertex Shader
	sVertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(sVertex, 3, vshader, NULL);
	glCompileShader(sVertex);

	// Fragment Shader
	sFragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(sFragment, 3, fshader, NULL);
	glCompileShader(sFragment);

	// Shader Program
	shader->queue = glCreateProgram();
	if (shader_compile_error(sVertex))
		glAttachShader(shader->queue, sVertex);
	else success++;
	if (shader_compile_error(sFragment))
		glAttachShader(shader->queue, sFragment);
	else success++;
	glLinkProgram(shader->queue);
	if (!program_compile_error(shader->queue))
		success++;

	glDeleteShader(sVertex);
	glDeleteShader(sFragment);

	return success;
}

int shader_use(shader_t* shader) {
	int change = (shader->queue != current);
	if (change) {
		glUseProgram(shader->queue);
		current = shader->queue;
	}
	return change;
}

void shader_delete(shader_t* shader) {
	glDeleteProgram(shader->queue);
}

static int shader_compile_error(GLuint obj) {
	int success;
	char infoLog[1024];
	glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(obj, 1024, NULL, infoLog);
		printf("Error compiling shader: %s \n", infoLog);
		return 0;
	}
	return 1;
}

static int program_compile_error(GLuint obj) {
	int success;
	char infoLog[1024];
	glGetProgramiv(obj, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(obj, 1024, NULL, infoLog);
		printf("Error linking shader: %s \n", infoLog);
		return 0;
	}
	return 1;
}

//Shader(path)
int w_shader_new(lua_State* L) {
	lua_pushinteger(L, RES_SHADER);
	return w_resource_new(L);
}

//Shader:__load(data)
int w_shader_load(lua_State* L) {
	shader_t* shader = (shader_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	char* data = luaL_checkstring(L, 2);
	shader_compile(shader, data);
	return 0;
}

//Shader:unload()
int w_shader_unload(lua_State* L) {
	shader_t* shader = (shader_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	shader_delete(shader);
	return 0;
}