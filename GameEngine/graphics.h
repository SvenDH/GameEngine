#pragma once
#include "memory.h"
#include "resource.h"
#include <lua.h>
#include <glad/glad.h>

#define Graphics_mt "graphics"
#define Canvas_mt "canvas"

#define BUFFER_FRAMES 3

typedef struct {
	texture_t texture;
	GLuint fbo;
	GLuint rbo;
} Canvas;

typedef struct {
	char* stream[2];
} VertexData;

typedef struct {
	int quad_count;
	texture_t* texture;
	shader_t* shader;
} DrawCommand;

typedef struct {
	GLuint vbo;
	GLsync syncs[BUFFER_FRAMES];
	char* data;
	size_t size;
	size_t offset;
	int index;
} StreamBuffer;

typedef struct {
	int quad_count;
	texture_t* texture;
	shader_t* shader;
	StreamBuffer buffer[2];
	VertexData map;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	uint16_t indexes[2];
} StreamState;

typedef struct {
	int width, height;
	mat3 projection;
	GLuint shader;
	GLuint canvas;
	//Color
} DisplayState;

typedef struct {
	ObjectAllocator textures;
	ObjectAllocator shaders;
	ObjectAllocator canvases;

	texture_t* default_texture;
	shader_t* default_shader;

	DisplayState display_state;
	StreamState draw_state;
	GLuint quad_indices;

	int draw_calls;
} graphics_t;

void graphics_init(graphics_t* gfx, const char* name);
void graphics_set_attributes(graphics_t* gfx, const StreamBuffer* pos_buffer, const StreamBuffer* mat_buffer);
void graphics_clear(graphics_t* gfx);
void graphics_resize(graphics_t* gfx, int width, int height, int zoom);
void graphics_flush(graphics_t* gfx);
VertexData graphics_requestdraw(graphics_t* gfx, const DrawCommand* cmd);
void graphics_present(graphics_t* gfx);
void graphics_stencil(graphics_t* gfx, int stencil);
void graphics_draw_quad(graphics_t* gfx, texture_t* texture, mat3 transform, int i, unsigned int color, float alpha);
void graphics_draw_text(graphics_t* gfx, texture_t* texture, const char* text, float x, float y, int center, float a);
Canvas* graphics_newcanvas(graphics_t* gfx, int width, int height);

inline GLOBAL_SINGLETON(graphics_t, graphics, "Gfx");

void streambuffer_init(StreamBuffer* buffer, size_t size);
void* streambuffer_map(StreamBuffer* buffer);
size_t streambuffer_unmap(StreamBuffer* buffer, size_t used);
void streambuffer_nextframe(StreamBuffer* buffer);

void texture_load(texture_t* texture, image_t* image, int width, int height);
void texture_unload(texture_t* texture);
void texture_generate(texture_t* texture, int tex_w, int tex_h, int tex_d, int channels, const char* data);
void texture_sheet(texture_t* texture, int sheet_w, int sheet_h, const char* data);
void texture_subimage(texture_t* texture, int depth, const char* data);
int texture_bind(texture_t* texture);

int shader_compile(shader_t* shader, const char* data);
void shader_delete(shader_t* shader);
int shader_use(shader_t* shader);

void canvas_init(Canvas* canvas, int width, int height);
int canvas_bind(Canvas* canvas);

int w_graphics_resize(lua_State* L);
int w_graphics_quad(lua_State* L);
int w_graphics_texture(lua_State* L);
int w_graphics_text(lua_State* L);

int openlib_Graphics(lua_State* L);

static char default_texture_data[] = { 255, 255, 255, 255 };
static char default_shader_data[] =
"#ifdef VERTEX_PROGRAM\n"
"layout(location = 0) in vec4 vert;"
"layout(location = 1) in mat3 model;"
"layout(location = 4) in vec4 color;"
"layout(location = 5) in float index;"
"out vec2 TexCoords;"
"out vec4 Color;"
"flat out float Index;"
"uniform mat3 projection;"
"void main() {"
"	vec3 pos = projection * model * vec3(vert.xy, 1.0);"
"	gl_Position = vec4(pos.xy, 0.0, 1.0);"
"	TexCoords = vert.zw;"
"	Color = color;"
"	Index = index;"
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

// Colors
#define COLOR_RANGE 0x90
#define MAX_COLOR 0xFF;

enum color {
	BLACK = 0x000000,		//0x80
	DARK_BLUE = 0x0000AA,	//0x81
	DARK_GREEN = 0x00AA00,	//0x82
	DARK_AQUA = 0x00AAAA,	//0x83
	DARK_RED = 0xAA0000,	//0x84
	DARK_PURPLE = 0xAA00AA,	//0x85
	GOLD = 0xFFAA00,		//0x86
	GRAY = 0xAAAAAA,		//0x87
	DARK_GRAY = 0x555555,	//0x88
	BLUE = 0x5555FF,		//0x89
	GREEN = 0x55FF55,		//0x8A
	AQUA = 0x55FFFF,		//0x8B
	RED = 0xFF5555,			//0x8C
	LIGHT_PURPLE = 0xFF55FF,//0x8D
	YELLOW = 0xFFFF55,		//0x8E
	WHITE = 0xFFFFFF,		//0x8F
};

static const unsigned int color_table[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_AQUA, DARK_RED, DARK_PURPLE, GOLD, GRAY, DARK_GRAY, BLUE, GREEN, AQUA, RED, LIGHT_PURPLE, YELLOW, WHITE };

inline void material_set(col_vertex_t* mat, int idx, uint32_t color, float alpha) {
	mat->col[0] = ((color >> 16) & 0xFF);	//r
	mat->col[1] = ((color >> 8) & 0xFF);	//g
	mat->col[2] = (color & 0xFF);			//b
	mat->col[3] = (alpha * 0xFF);			//a
	mat->idx = idx;			//texturearray index
}