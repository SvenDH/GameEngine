#include "graphics.h"
#include "sprite.h"
#include "window.h"
#include "utils.h"

Camera* default_camera;
Camera* current_camera;

Canvas* current_canvas;

void camera_new(Camera* camera, float x, float y, float width, float height) {
	glm_vec3((vec4) { width, height, 0., 1. }, camera->size);
	glm_ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, camera->projection);
	glm_vec3((vec4) { x, y, 0., 1. }, camera->pos);
	glm_translate_make(camera->view, camera->pos);
	camera->shader = NULL;
	camera->canvas = NULL;
}

void camera_set(Camera* camera) {
	if (current_camera != camera) {
		if (camera) {
			mat4 mvp;
			glm_mul(camera->projection, camera->view, &mvp);
			shader_bind(camera->shader);
			glUniformMatrix4fv(glGetUniformLocation(shader_getcurrent()->shd_ID, "mvp"), 1, GL_FALSE, &mvp);
			canvas_bind(camera->canvas);
		}
		else {
			shader_bind(NULL);
			glUniformMatrix4fv(glGetUniformLocation(shader_getcurrent()->shd_ID, "mvp"), 1, GL_FALSE, GLM_MAT4_IDENTITY);
			canvas_bind(NULL);
		}
		current_camera = camera;
	}
}

//Graphics.init(width, height, zoom)
int graphics_init(lua_State* L) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	return graphics_resize(L);
}

//Graphics.resize(width, height, zoom)
int graphics_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	int zoom = luaL_checkinteger(L, 3);

	glViewport(0, 0, width, height);
	default_camera = malloc(sizeof(Camera));
	camera_new(default_camera, 0, 0, (float)width / zoom, (float)height / zoom);
	//default_camera->canvas = malloc(sizeof(Canvas));
	//canvas_new(default_camera->canvas, width, height);

	return 0;
}

//Graphics.clear()
int graphics_clear(lua_State* L) {
	camera_set(default_camera);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glStencilMask(0x00);
	return 0;
}

//Graphics.stencil(0x00 - 0xFF)
int graphics_stencil(lua_State* L) {
	int stencil = luaL_checkinteger(L, 1);
	glStencilMask(stencil);
}

//Graphics.end()
int graphics_present(lua_State* L) {
	sprite_flush();
	window_swap();
	return 0;
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

static int canvas_get_texture(lua_State* L) {
	Canvas *canvas = luaL_checkudata(L, 1, Canvas_mt);
	lua_pushlightuserdata(L, canvas->tex);
	luaL_getmetatable(L, Texture_mt);
	lua_setmetatable(L, -2);
	return 1;
}

//Canvas(w, h)
static int canvas_new(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	if (width < 0 || height < 0) return luaL_error(L, "Error canvas has invalid dimensions");

	Canvas* canvas = (Canvas*)lua_newuserdata(L, sizeof(Canvas));
	luaL_setmetatable(L, Shader_mt);

	//Create Frame Buffer Object
	glGenFramebuffers(1, &canvas->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, canvas->FBO);
	//Create Screen Texture
	Texture* screentex = malloc(sizeof(Texture));
	texture_generate(screentex, width, height, 1, 4, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screentex->tex_ID, 0);
	//Create Render Buffer Object
	glGenRenderbuffers(1, &canvas->RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, canvas->RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas->RBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer is not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return 1;
}

static int canvas_set(lua_State* L) {
	Canvas *canvas = luaL_checkudata(L, 1, Canvas_mt);
	canvas_bind(canvas);
	return 0;
}

int openlib_Graphics(lua_State* L) {
	static luaL_Reg canv_func[] = {
		{"set", canvas_set},
		{NULL, NULL}
	};
	create_lua_class(L, Canvas_mt, canvas_new, canv_func);
	static const struct luaL_Reg gfx_lib[] = {
			{"init", graphics_init},
			{"resize", graphics_resize},
			{"clear", graphics_clear},
			{"stencil", graphics_stencil},
			{"present", graphics_present},
			{NULL, NULL}
	};
	create_lua_lib(L, Graphics_mt, gfx_lib);
	return 0;
}