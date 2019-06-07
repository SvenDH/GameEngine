#include "canvas.h"

Canvas* current_canvas;

void canvas_new(Canvas* canvas, int width, int height) {
	//Create Frame Buffer Object
	glGenFramebuffers(1, &canvas->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, canvas->FBO);
	//Create Screen Texture
	Texture* screentex = malloc(sizeof(Texture));
	texture_generate(screentex, width, height, 1, 4, NULL);
	sprite_new_ext(&canvas->spr, screentex, 0, -1, -1, 2, 2, 0, 0, 1, 1, 0xFFFFFF, 1);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screentex->tex_ID, 0);
	//Create Render Buffer Object
	glGenRenderbuffers(1, &canvas->RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, canvas->RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas->RBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer is not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void canvas_bind(Canvas* canvas) {
	if (canvas != current_canvas) {
		if (canvas) {
			glBindFramebuffer(GL_FRAMEBUFFER, canvas->FBO);
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		current_canvas = canvas;
	}
}

//Canvas.new(w, h)
static int lua_Canvas_new(lua_State* L) {
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);
	if (w < 0 || h < 0) return luaL_error(L, "Error canvas has invalid dimensions");

	Canvas* canvas = (Canvas*)lua_newuserdata(L, sizeof(Canvas));
	luaL_setmetatable(L, Shader_mt);

	return 1;
}

static int lua_Canvas_bind(lua_State* L) {
	Canvas *canvas = luaL_checkudata(L, 1, Canvas_mt);
	canvas_bind(canvas);
	return 0;
}

static luaL_Reg func[] = {
		{"bind", lua_Canvas_bind},
		{NULL, NULL}
};

static luaL_Reg lib[] = {
		{"new", lua_Canvas_new},
		{NULL, NULL}
};

int openlib_Canvas(lua_State* L) {
	luaL_newmetatable(L, Canvas_mt);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, func, 0);
	lua_pop(L, 1);

	luaL_newlib(L, lib);
	lua_setglobal(L, Canvas_mt);
	return 1;
}
