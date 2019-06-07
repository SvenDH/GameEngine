#include "shader.h"

static Shader* current_shader = NULL;
static Shader* default_shader;

static char* default_shader_source =
	"#ifdef VERTEX_PROGRAM\n"
	"layout(location = 0) in vec2 aPos;"
	"layout(location = 1) in vec2 aTex;"
	"layout(location = 2) in float aIdx;"
	"layout(location = 3) in vec4 aCol;"
	"out vec2 TexCoords;"
	"out vec4 Color;"
	"flat out float Index;"
	"uniform mat4 mvp;"
	"void main() {"
	"	gl_Position = mvp * vec4(aPos, 0.0, 1.0);"
	"	TexCoords = aTex;"
	"	Color = aCol;"
	"	Index = aIdx;"
	"}\n"
	"#endif\n"
	"#ifdef FRAGMENT_PROGRAM\n"
	"in vec2 TexCoords;"
	"in vec4 Color;"
	"flat in float Index;"
	"uniform sampler2DArray image;"
	"void main() {"
	"	vec4 tex = texture(image, vec3(TexCoords.xy, Index));"
	"	if (tex.a == 0) discard;"
	"	gl_FragColor = tex * Color;"
	"}\n"
	"#endif\n";

static int shader_compile_error(GLuint obj);
static int program_compile_error(GLuint obj);


int shader_compile(Shader *shader, const GLchar *source) {
	GLuint sVertex, sFragment;

	int success = 0;

	char *version = "#version 330\n";
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
	shader->shd_ID = glCreateProgram();
	if (shader_compile_error(sVertex))
		glAttachShader(shader->shd_ID, sVertex);
	else success++;
	if (shader_compile_error(sFragment))
		glAttachShader(shader->shd_ID, sFragment);
	else success++;
	glLinkProgram(shader->shd_ID);
	if (!program_compile_error(shader->shd_ID))
		success++;

	glDeleteShader(sVertex);
	glDeleteShader(sFragment);

	return success;
}

void shader_bind(Shader* shader) {
	if (!shader) {
		if (!default_shader) {
			default_shader = malloc(sizeof(Shader));
			shader_compile(default_shader, default_shader_source);
		}
		shader = default_shader;
	}
	if (shader != current_shader) {
		glUseProgram(shader->shd_ID);
		current_shader = shader;
	}
}

Shader* shader_getcurrent() {
	return current_shader;
}

static int shader_compile_error(GLuint obj) {
	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(obj, 1024, NULL, infoLog);
		printf("Error compiling shader: %s \n", infoLog);
		return 0;
	}
	else
		return 1;
}

static int program_compile_error(GLuint obj) {
	GLint success;
	GLchar infoLog[1024];
	glGetProgramiv(obj, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(obj, 1024, NULL, infoLog);
		printf("Error linking shader: %s \n", infoLog);
		return 0;
	}
	else return 1;
}

static int shader_load(lua_State* L) {
	int data_len;
	const char* data = luaL_checklstring(L, 1, &data_len);
	Shader* shd = (Shader*)lua_newuserdata(L, sizeof(Shader));
	luaL_setmetatable(L, Shader_mt);
	if (shader_compile(shd, data))
		return luaL_error(L, "Error compiling shader");
	else return 1;
}

static int shader_use(lua_State* L) {
	Shader *shader = (Shader*)luaL_checkudata(L, 1, Shader_mt);
	shader_bind(shader);
}

static int shader_delete(lua_State* L) {
	Shader *shader = (Shader*)luaL_checkudata(L, 1, Shader_mt);
	glDeleteProgram(shader->shd_ID);
}

static luaL_Reg shd_lib[] = {
		{"load", shader_load},
		{NULL, NULL}
};

static luaL_Reg shd_func[] = {
		{"use", shader_use},
		{"__gc", shader_delete},
		{NULL, NULL},
};

int openlib_Shader(lua_State* L) {
	luaL_newmetatable(L, Shader_mt);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, shd_func, 0);
	lua_pop(L, 1);

	luaL_newlib(L, shd_lib);
	lua_setglobal(L, Shader_mt);
	return 1;
}