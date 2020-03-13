#include "graphics.h"
#include "math.h"

static GLuint current_camera = 0;

camera_t* camera_new(graphics_t* gfx, UID id) {
	assert(!hashmap_get(&gfx->cameramap, id));
	camera_t* cam = object_alloc(&gfx->cameras, sizeof(camera_t));
	cam->id = id;
	cam->rbo = 0;
	cam->texture = NULL_RES;
	cam->shader = NULL_RES;
	memcpy(&cam->viewport, &NULL_RECT, sizeof(rect2));
	//Create Frame Buffer Object
	glGenFramebuffers(1, &cam->fbo);
	
	hashmap_put(&gfx->cameramap, id, cam);

	return cam;
}

void camera_set(graphics_t* gfx, UID id, rid_t shader, rid_t texture, rect2 viewport) {
	camera_t* cam = hashmap_get(&gfx->cameramap, id);
	if (texture.value != cam->texture.value) {
		cam->texture = texture;
		//Check if viewport size changed
		glBindFramebuffer(GL_FRAMEBUFFER, cam->fbo);
		glDeleteRenderbuffers(1, &cam->rbo);

		texture_t* tex = (texture_t*)resource_get(resourcemanager_instance(), cam->texture);
		texture_bind(tex);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex->queue, 0);
		//Create Render Buffer Object
		glGenRenderbuffers(1, &cam->rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, cam->rbo);
		//Allocate width x height renderbuffer with 24 bits depth and 8bits stencil (could be only stencil)
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tex->width, tex->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, cam->rbo);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Framebuffer is not complete!");
		glBindFramebuffer(GL_FRAMEBUFFER, current_camera);
	}
	if (shader.value != cam->shader.value) {
		cam->shader = shader;
	}
	memcpy(&cam->viewport, viewport, sizeof(rect2));
}

void camera_delete(graphics_t* gfx, UID id) {
	camera_set(gfx, id, NULL_RES, NULL_RES, &NULL_RECT);
	camera_t* cam = hashmap_get(&gfx->cameramap, id);
	glDeleteFramebuffers(1, &cam->fbo);
	object_free(&gfx->cameras, cam);
	hashmap_remove(&gfx->cameramap, id);
}

int camera_bind(graphics_t* gfx, camera_t* cam) {
	GLuint fbo = 0;
	if (cam) fbo = cam->fbo;
	int change = (fbo != current_camera);
	if (change) {
		graphics_flush(gfx);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		if (cam) {
			tranform_ortho(gfx->display_state.projection, cam->viewport[0], cam->viewport[2], cam->viewport[1], cam->viewport[3]);
			//TODO: can this be more efficient?
			shader_use(resource_get(resourcemanager_instance(), cam->shader));
		}
		current_camera = fbo;
	}
	return change;
}

int camera_update(system_t* system, event_t evt) {
	ent_transform* t;
	ent_camera* cam;
	type_t type = (type_t)evt.p0.data;
	entity_t ent = (entity_t)evt.p1.data;
	graphics_t* gfx = (graphics_t*)system->data;
	float x, y;
	switch (evt.type) {
	case on_addcomponent:
		if (type == comp_camera) {
			cam = entity_getcomponent(system->world, ent, comp_camera);
			cam->shader = gfx->default_shader;
			cam->texture = gfx->default_screen;
			camera_new(gfx, ent);
		}
		break;
	case on_setcomponent:
		t = entity_getcomponent(system->world, ent, comp_transform);
		cam = entity_getcomponent(system->world, ent, comp_camera);
		if (t) {
			x = t->position[0];
			y = t->position[1];
		}
		else {
			x = 0.0f;
			y = 0.0f;
		}
		camera_set(gfx, ent, cam->shader,
			cam->texture, (rect2) {
			x - cam->size[0] / 2, y - cam->size[1] / 2,
				x + cam->size[0] / 2, y + cam->size[1] / 2
		});
		break;
	case on_delcomponent:
		if (type == comp_camera)
			camera_delete(gfx, ent);
		break;
	}
	return 0;
}