#include "sprite.h"
#include "utils.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define COLOR_RANGE 0x90

vertex* mapped;

GLuint VBO;
GLuint EBO;
GLuint VAO;

static int next = 0;
static int init = 0;

const unsigned int color_table[] = { BLACK, DARK_BLUE, DARK_GREEN, DARK_AQUA, DARK_RED, DARK_PURPLE, GOLD, GRAY, DARK_GRAY, BLUE, GREEN, AQUA, RED, LIGHT_PURPLE, YELLOW, WHITE };

//Initialize the vertex buffer and index buffer on the gpu 
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

//Create quad (4 vertices), vert should have the size of atleast 4 vertices
inline void quad_new(vertex* vert, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c0, unsigned int c1, unsigned int c2, unsigned int c3, float a0, float a1, float a2, float a3) {
	vert[0].x = x;		vert[0].y = y;		vert[0].s = u;		vert[0].t = v;		vert[0].r = (c0 >> 16 & 0xFF); vert[0].g = (c0 >> 8 & 0xFF); vert[0].b = (c0 & 0xFF); vert[0].a = a0 * 0xFF; vert[0].i = i;
	vert[1].x = x;		vert[1].y = y + h;	vert[1].s = u;		vert[1].t = v + t;	vert[1].r = (c1 >> 16 & 0xFF); vert[1].g = (c1 >> 8 & 0xFF); vert[1].b = (c1 & 0xFF); vert[1].a = a1 * 0xFF; vert[1].i = i;
	vert[2].x = x + w;	vert[2].y = y + h;	vert[2].s = u + s;	vert[2].t = v + t;	vert[2].r = (c2 >> 16 & 0xFF); vert[2].g = (c2 >> 8 & 0xFF); vert[2].b = (c2 & 0xFF); vert[2].a = a2 * 0xFF; vert[2].i = i;
	vert[3].x = x + w;	vert[3].y = y;		vert[3].s = u + s;	vert[3].t = v;		vert[3].r = (c3 >> 16 & 0xFF); vert[3].g = (c3 >> 8 & 0xFF); vert[3].b = (c3 & 0xFF); vert[3].a = a3 * 0xFF; vert[3].i = i;
}

//If it doesnt exist yet, create buffer on cpu that will be flushed to gpu regularly
void* sprite_map() {
	if (!mapped)
		mapped = malloc(VERTEX_BUFFER_SIZE * 4 * sizeof(vertex));
	return mapped;
}

//Copy cpu vertex buffer to gpu
int sprite_unmap() {
	if (mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * VERTEX_BUFFER_SIZE * 4, NULL, GL_STREAM_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, next * 4 * sizeof(vertex), mapped);
		return 1;
	}
	else return 0;
}

void sprite_draw_tiles(Texture* tex, const char* tiles, float x, float y, unsigned int columns, unsigned int rows) {
	static vertex vertices[VERTEX_BUFFER_SIZE * 4];
	assert(columns * rows < VERTEX_BUFFER_SIZE);
	int i, j;

	int tile_w = tex->width;
	int tile_h = tex->height;

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {
			quad_new(&vertices[i * columns + j], tiles[i * columns + j], x + j * tile_w, y + i * tile_h, tile_w, tile_h, 0, 0, 1, 1, WHITE, WHITE, WHITE, WHITE, 1, 1, 1, 1);
		}
	}
}

void sprite_draw_text(Texture* tex, const char* text, float x, float y, int center, float a) {
	static vertex vertices[VERTEX_BUFFER_SIZE * 4];
	//assert(strlen(text) < VERTEX_BUFFER_SIZE);

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
					quad_new(&vertices[i * 4], ' ' - 32, xoff, yoff, tex->width, tex->height, 0, 0, 1, 1, color, color, color, color, a, a, a, a);
					xoff += tex->width;
					i++;
				}
				break;
			}
		}
		else if (*c < 0x80) {
			quad_new(&vertices[i * 4], *c - 32, xoff, yoff, tex->width, tex->height, 0, 0, 1, 1, color, color, color, color, a, a, a, a);
			xoff += tex->width;
			i++;
		}
		else if (*c < COLOR_RANGE) {
			color = color_table[*c & 0x0F];
		}
		c++;

		if (i == VERTEX_BUFFER_SIZE) {
			sprite_draw(tex, &vertices, i);
			i = 0;
		}
	}
	sprite_draw(tex, &vertices, i);
}

//Create new quad
void sprite_draw_quad(Texture* tex, unsigned int i, float x, float y, float w, float h, float u, float v, float s, float t, unsigned int c, float a) {
	vertex vertices[4];
	quad_new(&vertices[0], i, x, y, w, h, u, v, s, t, c, c, c, c, a, a, a, a);
	sprite_draw(tex, &vertices, 1);
}

//Draw sprite and, if texture changes or buffer is full first flush the buffer
void sprite_draw(Texture* texture, vertex* vertices, size_t size) {
	if (!init++) {
		sprite_init();
	}
	sprite_map();

	if (next >= VERTEX_BUFFER_SIZE || (texture_getcurrent() != texture))
		sprite_flush();

	texture_bind(texture);
	sprite_draw_vertices(vertices, size);
}

//Copy vertices into buffer
void sprite_draw_vertices(vertex* vertices, size_t size) {	
	memcpy(&mapped[next * 4], vertices, size * 4 * sizeof(vertex));
	next += size;
}

//Flush vertices and draw triangles
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

//Sprite.quad("Texture", x, y [, i, w, h, c, a])
static int sprite_quad(lua_State* L) {
	float x, y, w, h, a;
	int i, c;
	Texture* tex = NULL;
	Texture** ref = (Texture**)luaL_testudata(L, 1, Texture_mt);
	if (ref) tex = *ref;

	x = luaL_checkinteger(L, 2);
	y = luaL_checkinteger(L, 3);
	i = (int)luaL_optinteger(L, 4, 0);
	if (i < 0) return luaL_error(L, "Error incorrect sprite index");
	if (tex) w = (float)tex->width; else w = 1.0;
	w = (float)luaL_optnumber(L, 5, w);
	if (tex) h = (float)tex->height; else h = 1.0;
	h = (float)luaL_optnumber(L, 6, h);
	c = (int)luaL_optinteger(L, 7, 0xFFFFFF);
	a = (float)luaL_optnumber(L, 8, 1.0);

	sprite_draw_quad(tex, i, x, y, w, h, 0, 0, 1, 1, c, a);

	return 0;
}

//Sprite.text("Texture", text, x, y [, center, alpha])
static int sprite_text(lua_State* L) {
	int x, y, center;
	float a;
	Texture* tex = *(Texture**)luaL_checkudata(L, 1, Texture_mt);

	char* string = luaL_checkstring(L, 2);
	x = luaL_checkinteger(L, 3);
	y = luaL_checkinteger(L, 4);
	center = (int)luaL_optinteger(L, 5, 0);
	a = (float)luaL_optnumber(L, 6, 1.0);

	sprite_draw_text(tex, string, x, y, center, a);

	return 0;
}

int openlib_Sprite(lua_State* L) {
	//TODO: Sprite as class
	static luaL_Reg sprite_lib[] = {
		{"quad", sprite_quad},
		{"text", sprite_text},
		{NULL, NULL}
	};

	create_lua_lib(L, Sprite_mt, sprite_lib);
	return 0;
}