#include "graphics.h"
#include "memory.h"
#include "window.h"
#include "utils.h"
#include "math.h"

#include <lauxlib.h>
#include <lualib.h>

#define MAX_VERTICES 65535
#define MAX_QUADS MAX_VERTICES / 4
#define VERTEX_BUFFER_SIZE MAX_QUADS * sizeof(pos_vertex_t)
#define MATERIAL_BUFFER_SIZE MAX_QUADS * sizeof(col_vertex_t)

void graphics_init(graphics_t* gfx, const char* name) {
	objectallocator_init(&gfx->canvases, name, sizeof(Canvas), 8, 4);

	gfx->display_state.width = 0;
	gfx->display_state.height = 0;

	gfx->draw_state.texture = NULL;
	gfx->draw_state.shader = NULL;
	gfx->draw_state.quad_count = 0;
	gfx->draw_state.map.stream[0] = NULL;
	gfx->draw_state.map.stream[1] = NULL;

	streambuffer_init(&gfx->draw_state.buffer[0], VERTEX_BUFFER_SIZE);
	streambuffer_init(&gfx->draw_state.buffer[1], MATERIAL_BUFFER_SIZE);

	glGenVertexArrays(1, &gfx->draw_state.vao);
	glBindVertexArray(gfx->draw_state.vao);

	static const GLfloat vertex_buffer_data[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
	};
	glGenBuffers(1, &gfx->draw_state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gfx->draw_state.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

	graphics_set_attributes(gfx, &gfx->draw_state.buffer[0], &gfx->draw_state.buffer[1]);

	resourcemanager_t* rm = resourcemanager_instance();
	gfx->default_texture = resource_new(rm, hash_string("default texture"), RES_TEXTURE, 0);
	texture_generate(gfx->default_texture, 1, 1, 1, 4, default_texture_data);
	gfx->default_texture->loaded = 1;
	
	gfx->default_shader = resource_new(rm, hash_string("default shader"), RES_SHADER, 0);
	shader_compile(gfx->default_shader, default_shader_data);
	gfx->default_shader->loaded = 1;

	gfx->draw_calls = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void graphics_set_attributes(graphics_t* gfx, const StreamBuffer* pos_buffer, const StreamBuffer* mat_buffer) {
	//TODO: make this configurable
	size_t offset;
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, gfx->draw_state.vbo);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0); //x, y, u, v coords

	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer->vbo);
	offset = pos_buffer->offset;
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (void*)offset);					//mat3 row 1
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (void*)(offset + sizeof(vec3)));		//mat3 row 2
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (void*)(offset + 2 * sizeof(vec3)));	//mat3 row 3

	glBindBuffer(GL_ARRAY_BUFFER, mat_buffer->vbo);
	offset = mat_buffer->offset;
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(col_vertex_t), (void*)offset); //colors
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(col_vertex_t), (void*)(offset + sizeof(color_t))); //index

	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
}

void graphics_clear(graphics_t* gfx) {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	graphics_stencil(gfx, 0x00);
}

void graphics_resize(graphics_t* gfx, int width, int height, int zoom) {
	gfx->display_state.width = width;
	gfx->display_state.height = height;
	glViewport(0, 0, width, height);
	tranform_ortho(gfx->display_state.projection, 0, width / zoom, height / zoom, 0);
}

void graphics_stencil(graphics_t* gfx, int stencil) {
	glStencilMask(stencil);
}

VertexData graphics_requestdraw(graphics_t* gfx, const DrawCommand* cmd) {
	StreamState* state = &gfx->draw_state;
	int shouldflush = 0;
	int totalquads = state->quad_count + cmd->quad_count;

	if (totalquads > MAX_QUADS
		|| cmd->texture != state->texture
		|| cmd->shader != state->shader) {
		graphics_flush(gfx);
		state->texture = cmd->texture;
		state->shader = cmd->shader;
	}

	size_t sizes[2] = { 
		cmd->quad_count * sizeof(pos_vertex_t), 
		cmd->quad_count * sizeof(col_vertex_t) };

	VertexData d;
	for (int i = 0; i < 2; i++) {
		if (!state->map.stream[i])
			state->map.stream[i] = streambuffer_map(&state->buffer[i]);

		d.stream[i] = state->map.stream[i];
		state->map.stream[i] += sizes[i];
	}
	state->quad_count += cmd->quad_count;

	return d;
}

void graphics_flush(graphics_t* gfx) {
	StreamState* state = &gfx->draw_state;
	if (state->quad_count == 0)
		return;

	size_t offsets[2];
	size_t usedbytes[2] = {
		state->quad_count * sizeof(pos_vertex_t),
		state->quad_count * sizeof(col_vertex_t) };

	//TODO: check changed shader parameters
	shader_use(state->shader);
	glUniformMatrix3fv(glGetUniformLocation(state->shader->ID, "projection"), 1, GL_FALSE, &gfx->display_state.projection);
	graphics_set_attributes(gfx, &gfx->draw_state.buffer[0], &gfx->draw_state.buffer[1]);
	glActiveTexture(GL_TEXTURE0);
	texture_bind(state->texture);

	for (int i = 0; i < 2; i++) {
		offsets[i] = streambuffer_unmap(&gfx->draw_state.buffer[i], usedbytes[i]);
		state->map.stream[i] = NULL;
	}
	
	if (state->quad_count > 1)
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, state->quad_count);
	else
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	state->quad_count = 0;
	gfx->draw_calls++;
}

void graphics_present(graphics_t* gfx) {
	graphics_flush(gfx);
	for (int i = 0; i < 2; i++)
		streambuffer_nextframe(&gfx->draw_state.buffer[i]);
	window_swap(window_instance());
	gfx->draw_calls = 0;
}

void graphics_draw_quad(graphics_t* gfx, texture_t* tex, mat3 transform, int index, unsigned int color, float alpha) {
	DrawCommand cmd = {1, tex, gfx->default_shader };
	VertexData vertices = graphics_requestdraw(gfx, &cmd);
	//transform_set(vertices.stream[0], 0, 0, 0, 1, 1, x, y, 0, 0);
	memcpy(vertices.stream[0], transform, sizeof(mat3));
	material_set(vertices.stream[1], index, color, alpha);
}

void graphics_draw_text(graphics_t* gfx, texture_t* tex, const char* text, float x, float y, int center, float alpha) {
	DrawCommand cmd = { textcharcount(text), tex, gfx->default_shader };
	VertexData vertices = graphics_requestdraw(gfx, &cmd);
	pos_vertex_t* pos = vertices.stream[0];
	col_vertex_t* col = vertices.stream[1];

	int width = tex->width;
	int height = tex->height;
	unsigned int color = WHITE;
	int xoff = x - center * linelen(text) * width / 2;
	int yoff = y;

	int i = 0;
	char* c = text;
	while (*c) {
		if (*c < 0x20) {
			switch (*c) {
			case '\n':
				xoff = x - center * linelen(c + 1) * width / 2;
				yoff += height;
				break;
			case '\t':
				xoff += width * TAB_LEN;
				break;
			}
		}
		else if (*c < 0x80) {
			transform_set(&pos[i], xoff, yoff, 0.0f, width, height, 0.0f, 0.0f, 0.0f, 0.0f);
			material_set(&col[i], *c - 32, color, alpha);
			i++;
			xoff += width;
		}
		else if (*c < COLOR_RANGE) {
			color = color_table[*c & 0x0F];
		}
		c++;
	}
}

Canvas* graphics_newcanvas(graphics_t* gfx, int width, int height) {
	Canvas* canvas = objectallocator_alloc(&gfx->canvases, sizeof(Canvas));
	canvas_init(canvas, width, height);
	return canvas;
}

//Graphics.resize(w, h, zoom)
int w_graphics_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	int zoom = luaL_checkinteger(L, 3);

	graphics_resize(graphics_instance(), width, height, zoom);
	return 0;
}

//Graphics.quad(x, y, w, h [, c [, a]])
int w_graphics_quad(lua_State* L) {
	float x = (int)luaL_checknumber(L, 1);
	float y = (int)luaL_checknumber(L, 2);
	float w = (int)luaL_checknumber(L, 3);
	float h = (int)luaL_checknumber(L, 4);
	int c = (int)luaL_optinteger(L, 5, 0xFFFFFF);
	float a = (float)luaL_optnumber(L, 6, 1.0);
	graphics_t* gfx = graphics_instance();
	mat3 transform;
	transform_set(transform, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f, 0.0f);
	graphics_draw_quad(gfx, gfx->default_texture, transform, 0, c, a);
	return 0;
}

//Graphics.texture("Texture", x, y [, c [, a[, i]]])
int w_graphics_texture(lua_State* L) {
	texture_t* texture = (texture_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	if (!texture->loaded) LOAD_RESOURCE(L, 1);
	float x = luaL_optnumber(L, 2, 0.0f);
	float y = luaL_optnumber(L, 3, 0.0f);
	int i = luaL_optinteger(L, 4, 0);
	int c = luaL_optinteger(L, 5, 0xFFFFFF);
	float a = luaL_optinteger(L, 6, 1.0f);
	mat3 transform;
	transform_set(transform, x, y, 0.0f, texture->width, texture->height, 0.0f, 0.0f, 0.0f, 0.0f);
	graphics_draw_quad(graphics_instance(), texture, transform, i, c, a);
	return 0;
}

//Graphics.text("Texture", text, x, y [, center, alpha])
int w_graphics_text(lua_State* L) {
	graphics_t* gfx = graphics_instance();
	int x, y, center;
	float a;
	texture_t* texture = (texture_t*)resource_get(resourcemanager_instance(), *(rid_t*)lua_touserdata(L, 1));
	if (!texture) texture = gfx->default_texture;
	else if (!texture->loaded) LOAD_RESOURCE(L, 1);
	char* string = luaL_checkstring(L, 2);
	x = luaL_checknumber(L, 3);
	y = luaL_checknumber(L, 4);
	center = (int)luaL_optinteger(L, 5, 0);
	a = (float)luaL_optnumber(L, 6, 1.0);

	graphics_draw_text(gfx, texture, string, x, y, center, a);
	return 0;
}

int openlib_Graphics(lua_State* L) {
	static const struct luaL_Reg gfx_lib[] = {
			{"resize", w_graphics_resize},
			{"clear", graphics_clear},
			{"stencil", graphics_stencil},
			{"flush", graphics_flush},
			{"quad", w_graphics_quad},
			{"text", w_graphics_text},
			{NULL, NULL}
	};
	create_lua_lib(L, Graphics_mt, gfx_lib);

	return 0;
}

/*
void graphics_set_indexbuffer(Graphics* gfx) {
	//TODO: this is only for quads, make this configurable
	uint16_t indices[MAX_QUADS * 6];
	for (int i = 0; i < MAX_QUADS; i++) {
		indices[i * 6 + 0] = 0 + (i * 4);
		indices[i * 6 + 1] = 1 + (i * 4);
		indices[i * 6 + 2] = 2 + (i * 4);

		indices[i * 6 + 3] = 0 + (i * 4);
		indices[i * 6 + 4] = 2 + (i * 4);
		indices[i * 6 + 5] = 3 + (i * 4);
	}

	glGenBuffers(1, &gfx->quad_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->quad_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUADS * 6 * sizeof(uint16_t), indices, GL_STATIC_DRAW);
}
*/