#include "graphics.h"

static GLuint current = 0;

void canvas_init(Canvas* canvas, int width, int height) {
	//Create Frame Buffer Object
	glGenFramebuffers(1, &canvas->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);
	//Create Screen Texture
	texture_generate(&canvas->texture, width, height, 1, 4, NULL);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, canvas->texture.ID, 0);
	//Create Render Buffer Object
	glGenRenderbuffers(1, &canvas->rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, canvas->rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas->rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer is not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, current);
}

int canvas_bind(Canvas* canvas) {
	int change = (canvas->fbo != current);
	if (change) {
		glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);
		current = canvas->fbo;
	}
	return change;
}