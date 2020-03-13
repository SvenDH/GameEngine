#include "graphics.h"
#include "memory.h"
#include "utils.h"
#include "math.h"
#include "ecs.h"

#include <lauxlib.h>
#include <lualib.h>

#define MAX_VERTICES 65535
#define MAX_QUADS MAX_VERTICES / 4
#define VERTEX_BUFFER_SIZE MAX_QUADS * sizeof(pos_vertex_t)
#define MATERIAL_BUFFER_SIZE MAX_QUADS * sizeof(col_vertex_t)

void graphics_init(graphics_t* gfx, const char* name) {
	object_init(&gfx->cameras, name, sizeof(camera_t), 8, 4);
	hashmap_init(&gfx->cameramap);

	gfx->display_state.width = 0;
	gfx->display_state.height = 0;

	gfx->draw_state.texture.value = 0;
	gfx->draw_state.shader.value = 0;
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
	gfx->default_screen = resource_new(rm, hash_string("default screen"), RES_TEXTURE, 0);
	gfx->default_shader = resource_new(rm, hash_string("default shader"), RES_SHADER, 0);

	texture_generate(resource_get(rm, gfx->default_texture), 1, 1, 1, 4, default_texture_data);
	shader_compile(resource_get(rm, gfx->default_shader), default_shader_data);

	gfx->draw_calls = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	world_t* world = world_instance();
	system_new(world, animation_update, 0, animation|sprite, disabled, postupdate, gfx);
	system_new(world, draw_update, 0, transform|sprite, disabled, draw, gfx);
	system_new(world, camera_update, transform|camera, transform|camera, disabled, addcomponent|setcomponent|delcomponent, gfx);
}

int draw_update(system_t* system, event_t evt) {
	type_t* type; type_data* arch; chunk_data* chunk;
	rid_t tex_id = NULL_RES, shd_id = NULL_RES;
	texture_t* tex = NULL; shader_t* shd;
	graphics_t* gfx = (graphics_t*)system->data;
	double dt = evt.p0.nr;
	mat3 transform; UID cam_id;  camera_t* cam;
	hashmap_foreach(&gfx->cameramap, cam_id, cam) {
		camera_bind(gfx, cam);
		graphics_clear(gfx);
		hashmap_foreach(&system->archetypes, type, arch) {
			archetype_foreach(system->world, arch, chunk) {
				ent_transform* t = chunk_get_componentarray(chunk, comp_transform);
				ent_sprite* spr = chunk_get_componentarray(chunk, comp_sprite);
				ent_tiles* tiles = chunk_get_componentarray(chunk, comp_tiles);
				for (int i = 0; i < chunk->ent_count; i++) {
					if (spr[i].texture.value != tex_id.value) {
						tex_id = spr[i].texture;
						tex = resource_get(resourcemanager_instance(), tex_id);
					}
					transform_set(transform,
						t[i].position[0] - spr[i].offset[0], t[i].position[1] - spr[i].offset[1],
						0.0f, tex->width, tex->height, 0.0f, 0.0f, 0.0f, 0.0f);
					
					if (tiles) {
						graphics_draw_tiles(gfx, tex_id, transform,
							tiles[i].data, TILE_ROWS, TILE_COLS,
							spr[i].color, spr[i].alpha);
					}
					else {
						graphics_draw_quad(gfx, tex_id, transform,
							spr[i].index, spr[i].color, spr[i].alpha);
					}
				}
			}
		}
	}
	camera_bind(gfx, NULL);
	tranform_ortho(gfx->display_state.projection, 0, gfx->display_state.width, gfx->display_state.height, 0);
	return 0;
}

int animation_update(system_t* system, event_t evt) {
	type_t* type; type_data* arch; chunk_data* chunk;
	double dt = evt.p0.nr;
	hashmap_foreach(&system->archetypes, type, arch) {
		archetype_foreach(system->world, arch, chunk) {
			ent_sprite* spr = chunk_get_componentarray(chunk, comp_sprite);
			ent_animation* anim = chunk_get_componentarray(chunk, comp_animation);
			for (int i = 0; i < chunk->ent_count; i++) {
				anim[i].index += anim[i].speed * dt;

				if (anim[i].speed > 0 && (
					anim[i].index > anim[i].end_index ||
					anim[i].index < anim[i].start_index))
					anim[i].index = anim[i].start_index;
				else if (anim[i].speed < 0 && (
					anim[i].index < anim[i].start_index ||
					anim[i].index >= anim[i].end_index))
					anim[i].index = anim[i].end_index - 1;

				spr[i].index = anim[i].index;
			}
		}
	}
	return 0;
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

void graphics_resize(graphics_t* gfx, int width, int height) {
	gfx->display_state.width = width;
	gfx->display_state.height = height;
	glViewport(0, 0, width, height);
	tranform_ortho(gfx->display_state.projection, 0, width, height, 0);
	texture_t* texture = (texture_t*)resource_get(resourcemanager_instance(), gfx->default_screen);
	texture_delete(texture);
	texture_generate(texture, width, height, 1, 4, NULL);
}

void graphics_stencil(graphics_t* gfx, int stencil) {
	glStencilMask(stencil);
}

VertexData graphics_requestdraw(graphics_t* gfx, const DrawCommand* cmd) {
	StreamState* state = &gfx->draw_state;
	int shouldflush = 0;
	int totalquads = state->quad_count + cmd->quad_count;
	if (totalquads > MAX_QUADS
		|| cmd->texture.value != state->texture.value
		|| cmd->shader.value != state->shader.value) {
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
		state->quad_count * sizeof(col_vertex_t)};

	resourcemanager_t* rm = resourcemanager_instance();
	//TODO: check changed shader parameters
	shader_t* shd = (shader_t*)resource_get(rm, state->shader);
	texture_t* tex = (texture_t*)resource_get(rm, state->texture);

	shader_use(shd);
	glUniformMatrix3fv(glGetUniformLocation(shd->queue, "projection"), 1, GL_FALSE, &gfx->display_state.projection);
	graphics_set_attributes(gfx, &gfx->draw_state.buffer[0], &gfx->draw_state.buffer[1]);
	glActiveTexture(GL_TEXTURE0);
	texture_bind(tex);

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

	mat3 transform;
	transform_set(transform, 0.0f, 0.0f, 0.0f, gfx->display_state.width, gfx->display_state.height, 0.0f, 0.0f, 0.0f, 0.0f);
	graphics_draw_quad(gfx, gfx->default_screen, transform, 0, 0xFFFFFF, 1.0f);
	
	graphics_flush(gfx);
	for (int i = 0; i < 2; i++)
		streambuffer_nextframe(&gfx->draw_state.buffer[i]);
	gfx->draw_calls = 0;
}

void graphics_draw_quad(graphics_t* gfx, rid_t texture, mat3 transform, int index, unsigned int color, float alpha) {
	DrawCommand cmd = {1, texture, gfx->default_shader };
	VertexData vertices = graphics_requestdraw(gfx, &cmd);
	memcpy(vertices.stream[0], transform, sizeof(mat3));
	material_set(vertices.stream[1], index, color, alpha);
}

void graphics_draw_tiles(graphics_t* gfx, rid_t texture, mat3 transform, uint8_t* tiles, int rows, int cols, unsigned int color, float alpha) {
	DrawCommand cmd = { rows * cols, texture, gfx->default_shader };
	VertexData vertices = graphics_requestdraw(gfx, &cmd);
	pos_vertex_t* pos = vertices.stream[0];
	col_vertex_t* col = vertices.stream[1];
	mat3 tile_transform;
	int i = 0;
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++, i++) {
			transform_set(tile_transform, x, y, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			transform_mul(&pos[i], transform, tile_transform);
			material_set(&col[i], tiles[y * cols + x], color, alpha);
		}
	}	
}

void graphics_draw_text(graphics_t* gfx, rid_t texture, mat3 transform, const char* text, int center, float alpha) {
	DrawCommand cmd = { textcharcount(text), texture, gfx->default_shader };
	VertexData vertices = graphics_requestdraw(gfx, &cmd);
	pos_vertex_t* pos = vertices.stream[0];
	col_vertex_t* col = vertices.stream[1];
	unsigned int color = WHITE;
	float xoff = - (center * linelen(text) / 2.0f);
	float yoff = 0.0f;
	mat3 tile_transform;
	int i = 0;
	char* c = text;
	while (*c) {
		if (*c < 0x20) {
			switch (*c) {
			case '\n':
				xoff =  - center * linelen(c + 1) / 2;
				yoff += 1;
				break;
			case '\t':
				xoff += TAB_LEN;
				break;
			}
		}
		else if (*c < 0x80) {
			transform_set(tile_transform, xoff, yoff, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			transform_mul(&pos[i], transform, tile_transform);
			material_set(&col[i], *c - 32, color, alpha);
			i++;
			xoff += 1;
		}
		else if (*c < COLOR_RANGE) {
			color = color_table[*c & 0x0F];
		}
		c++;
	}
}

//Graphics.resize(w, h)
int w_graphics_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	graphics_resize(graphics_instance(), width, height);
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
	resourcemanager_t* rm = resourcemanager_instance();

	rid_t tex_id = *(rid_t*)luaL_checkudata(L, 1, res_mt[RES_TEXTURE]);
	if (!resource_isloaded(rm, tex_id)) LOAD_RESOURCE(L, 1);
	float x = luaL_optnumber(L, 2, 0.0f);
	float y = luaL_optnumber(L, 3, 0.0f);
	int i = luaL_optinteger(L, 4, 0);
	int c = luaL_optinteger(L, 5, 0xFFFFFF);
	float a = luaL_optinteger(L, 6, 1.0f);

	mat3 transform;
	texture_t* texture = (texture_t*)resource_get(rm, tex_id);
	transform_set(transform, x, y, 0, texture->width, texture->height, 0, 0, 0, 0);
	graphics_draw_quad(graphics_instance(), tex_id, transform, i, c, a);
	return 0;
}

//Graphics.text("Texture", text, x, y [, center, alpha])
int w_graphics_text(lua_State* L) {
	resourcemanager_t* rm = resourcemanager_instance();

	rid_t font_id = *(rid_t*)luaL_checkudata(L, 1, res_mt[RES_TEXTURE]);
	if (!resource_isloaded(rm, font_id)) LOAD_RESOURCE(L, 1);
	char* string = luaL_checkstring(L, 2);
	int x = luaL_checknumber(L, 3);
	int y = luaL_checknumber(L, 4);
	int center = (int)luaL_optinteger(L, 5, 0);
	float a = (float)luaL_optnumber(L, 6, 1.0);

	mat3 transform;
	texture_t* font = (texture_t*)resource_get(rm, font_id);
	transform_set(transform, x, y, 0, font->width, font->height, 0, 0, 0, 0);
	graphics_draw_text(graphics_instance(), font_id, transform, string, center, a);
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