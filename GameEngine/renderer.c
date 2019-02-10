#include "renderer.h"
#include "resources.h"
#include "canvas.h"

#include <glad.h>
#include <cglm/cglm.h>

static int width, height, zoom;

Canvas canvas;

int renderer_init(lua_State* L) {
	zoom = luaL_checkinteger(L, -1);
	height = luaL_checkinteger(L, -2);
	width = luaL_checkinteger(L, -3);

	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	canvas_new(&canvas, width, height, zoom, NULL, NULL);
}

void renderer_start() {
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void renderer_begin() {
	canvas_bind(&canvas);
	glClearColor(1.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glStencilMask(0x00);
}

void renderer_end() {
	sprite_flush();

	canvas_bind(NULL);
	glStencilMask(0xFF);

	sprite_draw(&canvas);
	sprite_flush();
}

void renderer_finish() {}

int openlib_Render(lua_State* L) {
	static const struct luaL_Reg lib[] = {
		{"init", renderer_init},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	lua_setglobal(L, "Renderer");
	return 1;
}