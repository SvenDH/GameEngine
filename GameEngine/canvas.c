#include "canvas.h"
#include "resources.h"

#define camera_ctor(_cam, _x, _y, _w, _h) \
									_cam = malloc(sizeof(Camera)); \
									camera_new(_cam, _x, _y, _w, _h)

Canvas* current_canvas;

void canvas_new(Canvas* canvas, int width, int height, int zoom, Shader* shader, Camera* camera) {
	if (shader) canvas->shader = shader;
	else canvas->shader = resources_get("default shader");

	if (camera) canvas->camera = camera;
	else camera_ctor(canvas->camera, 0, 0, (float)width / zoom, (float)height / zoom);

	//Create Frame Buffer Object
	glGenFramebuffers(1, &canvas->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, canvas->FBO);
	//Create Screen Texture
	Texture* screentex = malloc(sizeof(Texture));
	texture_generate(screentex, width, height, 1, 4, NULL);
	sprite_new_ext(&canvas->spr, screentex, 0, -1, -1, 2, 2, 0, 0, 1, 1, 0xFFFFFF, 1);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screentex->tex_ID, 0);
	//Create Render Buffer Object
	GLuint RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer is not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void canvas_bind(Canvas* canvas) {
	if (canvas) {
		glBindFramebuffer(GL_FRAMEBUFFER, canvas->FBO);
		shader_use(NULL);
		mat4 mvp;
		glm_mul(canvas->camera->projection, canvas->camera->view, &mvp);
		glUniformMatrix4fv(glGetUniformLocation(shader_getcurrent()->shd_ID, "mvp"), 1, GL_FALSE, &mvp);
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		shader_use(current_canvas->shader);
		glUniformMatrix4fv(glGetUniformLocation(shader_getcurrent()->shd_ID, "mvp"), 1, GL_FALSE, GLM_MAT4_IDENTITY);
	}
	current_canvas = canvas;
}

void camera_new(Camera* camera, float x, float y, float width, float height) {
	glm_vec3((vec4) {width, height, 0., 1.}, camera->size);
	glm_ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f, camera->projection);
	glm_vec3((vec4) { x, y, 0., 1. }, camera->pos);
	glm_translate_make(camera->view, camera->pos);

}

