#include "shader.h"
#include "resources.h"

static Shader* current_shader = NULL;
static Shader* default_shader;

void shader_compile(Shader *shader, const GLchar *source) {
	GLuint sVertex, sFragment;

	GLint success;
	GLchar infoLog[1024];

	GLchar *version = "#version 330\n";
	GLchar *vshader[3] = { version, "#define VERTEX_PROGRAM\n", source };
	GLchar *fshader[3] = { version, "#define FRAGMENT_PROGRAM\n", source };

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
	if (shader_compile_error(sFragment))
		glAttachShader(shader->shd_ID, sFragment);
	glLinkProgram(shader->shd_ID);
	program_compile_error(shader->shd_ID);

	glDeleteShader(sVertex);
	glDeleteShader(sFragment);
}

void shader_delete(Shader* shader) {
	glDeleteProgram(shader->shd_ID);
}

void shader_use(Shader* shader) {
	if (!shader) shader = default_shader;
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
		printf("| ERROR::Shader: Compile-time error: %s \n", infoLog);
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
		printf("| ERROR::Shader: Link-time error: %s \n", infoLog);
		return 0;
	}
	else
		return 1;
}

char *default_shader_source =
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
	"   if (tex.a == 0) discard;"
	"	gl_FragColor = tex * Color;"
	"}\n"
	"#endif\n";


int openlib_Shader(lua_State* L) {
	RESOURCE_INIT((Resource*)default_shader, "default shader", SHADER);
	shader_compile(default_shader, default_shader_source);
	shader_use(default_shader);
	glUniform1i(glGetUniformLocation(default_shader->shd_ID, "image"), 0);

	luaL_newmetatable(L, Shader_mt);
	lua_pop(L, 1);
	return 1;
}