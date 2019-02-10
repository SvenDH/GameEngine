#include "sprite.h"
#include "utils.h"
#include "resources.h"

#include <string.h>
#include <stdlib.h>

vertex* mapped;

GLuint VBO;
GLuint EBO;
GLuint VAO;

int next;

const unsigned int color_table[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_AQUA, DARK_RED, DARK_PURPLE, GOLD, GRAY, DARK_GRAY, BLUE, GREEN, AQUA, RED, LIGHT_PURPLE, YELLOW, WHITE };

void sprite_init() {
	GLushort indices[VERTEX_BUFFER_SIZE * 6];
	for (int i = 0; i < VERTEX_BUFFER_SIZE; i++) {
		indices[i * 6 + 0] = 0 + (i * 4);
		indices[i * 6 + 1] = 1 + (i * 4);
		indices[i * 6 + 2] = 2 + (i * 4);

		indices[i * 6 + 3] = 0 + (i * 4);
		indices[i * 6 + 4] = 2 + (i * 4);
		indices[i * 6 + 5] = 3 + (i * 4);
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * VERTEX_BUFFER_SIZE * 4, NULL, GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * VERTEX_BUFFER_SIZE * 6, indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), 0); //x, y coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(2 * sizeof(float))); //t, s coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(4 * sizeof(float))); //index
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void*)(5 * sizeof(float))); //colors

	glBindVertexArray(0);
}

inline void quad_new(vertex* vert, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3, float a0, float a1, float a2, float a3) {
	vert[0].x = x;		vert[0].y = y;		vert[0].s = u;		vert[0].t = v;		vert[0].r = (c0 >> 16 & 0xFF); vert[0].g = (c0 >> 8 & 0xFF); vert[0].b = (c0 & 0xFF); vert[0].a = a0 * 0xFF; vert[0].i = i;
	vert[1].x = x;		vert[1].y = y + h;	vert[1].s = u;		vert[1].t = v + t;	vert[1].r = (c1 >> 16 & 0xFF); vert[1].g = (c1 >> 8 & 0xFF); vert[1].b = (c1 & 0xFF); vert[1].a = a1 * 0xFF; vert[1].i = i;
	vert[2].x = x + w;	vert[2].y = y + h;	vert[2].s = u + s;	vert[2].t = v + t;	vert[2].r = (c2 >> 16 & 0xFF); vert[2].g = (c2 >> 8 & 0xFF); vert[2].b = (c2 & 0xFF); vert[2].a = a2 * 0xFF; vert[2].i = i;
	vert[3].x = x + w;	vert[3].y = y;		vert[3].s = u + s;	vert[3].t = v;		vert[3].r = (c3 >> 16 & 0xFF); vert[3].g = (c3 >> 8 & 0xFF); vert[3].b = (c3 & 0xFF); vert[3].a = a3 * 0xFF; vert[3].i = i;
}

void sprite_new(Sprite* spr, Texture* tex, unsigned int i, float x, float y, float w, float h, unsigned int c, float a) {
	sprite_new_ext(spr, tex, i, x, y, w, h, 0, 0, 1, 1, c, a);
}

void sprite_new_ext(Sprite* spr, Texture* tex, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c, float a) {
	sprite_ctor(spr, tex, 1);
	quad_new(spr->vertices, i, x, y, w, h, u, v, s, t, c, c, c, c, a, a, a, a);
}

void sprite_new_text(Sprite* spr, Texture* tex, const char* text, float x, float y, int center, float a) {
	sprite_ctor(spr, tex, strlen(text));

	unsigned char* c = text;
	int color = WHITE;
	int xoff = x - center * linelen(c) * tex->width / 2;
	int yoff = y;
	int i = 0;
	while (*c) {
		if (*c < 0x20) {
			switch (*c) {
			case '\n':
				xoff = x - center * linelen(c + 1) * tex->width / 2;
				yoff += tex->height;
				break;
			case '\t':
				for (int i = 0; i < TAB_LEN; i++) {
					quad_new(&spr->vertices[i * 4], ' '-32, xoff, yoff, tex->width, tex->height, 0, 0, 1, 1, color, color, color, color, a, a, a, a);
					xoff += tex->width;
					i++;
				}
				break;
			}
		}
		else if (*c < 0x80) {
			quad_new(&spr->vertices[i * 4], *c-32, xoff, yoff, tex->width, tex->height, 0, 0, 1, 1, color, color, color, color, a, a, a, a);
			xoff += tex->width;
			i++;
		}
		else if (*c < 0x90) {
			color = color_table[*c & 0x0F];
		}
		c++;
	}
}

void* sprite_map() {
	if (!mapped)
		mapped = malloc(VERTEX_BUFFER_SIZE * 4 * sizeof(vertex));
	return mapped;
}

int sprite_unmap() {
	if (mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * VERTEX_BUFFER_SIZE * 4, NULL, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, next * 4 * sizeof(vertex), mapped);
		return 1;
	}
	else return 0;
}

void sprite_draw(Sprite* spr) {
	sprite_map();
	
	if (next >= VERTEX_BUFFER_SIZE || (spr->texture && texture_getcurrent() != spr->texture))
		sprite_flush();

	texture_bind(spr->texture);
	memcpy(&mapped[next * 4], spr->vertices, spr->size * 4 * sizeof(vertex));
	
	next += spr->size;
}

void sprite_flush() {
	if (next == 0)
		return;

	if (mapped && sprite_unmap()) {
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, next * 6, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);
	}
	next = 0;
}

Sprite *checksprite(lua_State *L, int i) {
	void *ud = luaL_checkudata(L, i, Sprite_mt);
	luaL_argcheck(L, ud != NULL, i, "Sprite expected");
	return (Sprite*)ud;
}

//Sprite:new("Texture", x, y, i)
static int lua_Sprite_new(lua_State* L) {
	int top = lua_gettop(L);
	Texture* tex = NULL;

	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char* tex_name = lua_tostring(L, 2);
		Resource* r = resources_get(tex_name);
		if (r->type == TEXTURE)
			tex = (Texture*)r;
	}
	else tex = checktexture(L, 2);
	

	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);
	int i = luaL_checkinteger(L, 5);
	int c = 0xFFFFFF;
	float a = 1.0;
	if (top > 5) c = (int)luaL_checkinteger(L, 6);
	if (top > 6) a = (float)luaL_checknumber(L, 7);
	lua_settop(L, 0);

	Sprite *spr = (Sprite*)lua_newuserdata(L, sizeof(Sprite));
	luaL_getmetatable(L, Sprite_mt);
	lua_setmetatable(L, 1);
	sprite_new(spr, tex, i, x, y, tex->width, tex->height, c, a);

	return 1;
}

//Sprite:text("Texture", x, y, text)
static int lua_Sprite_text(lua_State* L) {
	int top = lua_gettop(L);
	Texture* tex = NULL;

	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char* tex_name = lua_tostring(L, 2);
		Resource* r = resources_get(tex_name);
		if (r->type == TEXTURE)
			tex = (Texture*)r;
	}
	else tex = checktexture(L, 2);

	char* string = luaL_checkstring(L, 3);
	int x = luaL_checkinteger(L, 4);
	int y = luaL_checkinteger(L, 5);
	int center = 0;
	float a = 1.0;
	if (top > 5) center = (int)luaL_checkinteger(L, 6);
	if (top > 6) a = (float)luaL_checknumber(L, 7);
	lua_settop(L, 0);

	Sprite *spr = (Sprite*)lua_newuserdata(L, sizeof(Sprite));
	luaL_getmetatable(L, Sprite_mt);
	lua_setmetatable(L, 1);
	sprite_new_text(spr, tex, string, x, y, center, a);

	return 1;
}

//Sprite.__index
static int lua_Sprite_index(lua_State* L) {
	Sprite *s = checksprite(L, 1);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "texture")) {
			lua_pushlightuserdata(L, s->texture);
			luaL_getmetatable(L, Texture_mt);
			lua_setmetatable(L, -2);
		}
		else if (!strcmp(key, "count")) {
			lua_pushinteger(L, s->size);
		}
	}
	else lua_pushnil(L);
	return 1;
}

static int lua_Sprite_newindex(lua_State* L) {
	Sprite *s = checksprite(L, 1);
	int t = lua_type(L, 2);
	if (t == LUA_TSTRING) {
		const char *key = lua_tostring(L, 2);
		if (!strcmp(key, "i")) {
			int newi = luaL_checkinteger(L,3);
			for (int i = 0; i < s->size * 4; i++)
				s->vertices[i].i = newi;
		}
	}
}

static int lua_Sprite_delete(lua_State* L) {
	Sprite *s = checksprite(L, 1);
	free(s->vertices);
	free(s);
}

int openlib_Sprite(lua_State* L) {
	sprite_init();

	luaL_newmetatable(L, Sprite_mt);
	static const struct luaL_Reg lib[] = {
		{"__index", lua_Sprite_index},
		{"__newindex", lua_Sprite_newindex},
		{"__gc", lua_Sprite_delete},
		{NULL, NULL}
	};
	luaL_setfuncs(L, lib, 0);
	lua_pop(L, 1);
	lua_settop(L, 0);
	static luaL_Reg func[] = {
		{"new", lua_Sprite_new},
		{"text", lua_Sprite_text},
		{NULL, NULL}
	};
	luaL_newlib(L, func);
	lua_setglobal(L, Sprite_mt);
	return 1;
}