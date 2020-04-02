#include "graphics.h"
#include "physics.h"
#include "math.h"

#define CULLARRAY_SIZE 8192

SYSTEM_ENTRY(camera_new) {
	graphics_t* gfx = task->data;
	//Set defaults on cam
	COMPARRAY_GET(Camera, cam);
	for (int i = task->start; i < task->end; i++) {
		cam[i].texture = gfx->default_screen;
		cam[i].shader = gfx->default_shader;
		cam[i].size[0] = 640.0f;
		cam[i].size[1] = 480.0f;
		cam[i].rbo = 0;
		//Create Frame Buffer Object
		glGenFramebuffers(1, &cam[i].fbo);
	}
}

SYSTEM_ENTRY(camera_delete) {
	COMPARRAY_GET(Camera, cam);
	for (int i = task->start; i < task->end; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, cam[i].fbo);
		glDeleteRenderbuffers(1, &cam[i].rbo);
		glDeleteFramebuffers(1, &cam[i].fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

SYSTEM_ENTRY(camera_set) {
	COMPARRAY_GET(Camera, cam);
	for (int i = task->start; i < task->end; i++) {
		//Check if viewport size changed
		glBindFramebuffer(GL_FRAMEBUFFER, cam[i].fbo);
		glDeleteRenderbuffers(1, &cam->rbo);
		//Get texture to render to
		texture_t* tex = (texture_t*)resource_get(cam[i].texture);
		texture_bind(tex);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex->id, 0);
		//Create Render Buffer Object
		glGenRenderbuffers(1, &cam[i].rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, cam[i].rbo);
		//Allocate width x height renderbuffer with 24 bits depth and 8bits stencil (could be only stencil)
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tex->width, tex->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, cam[i].rbo);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			log_info("Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}


SYSTEM_ENTRY(camera_update) {
	graphics_t* gfx = task->data;
	COMPARRAY_GET(Transform, t);
	COMPARRAY_GET(Camera, cam);
	UID cull_ids[CULLARRAY_SIZE];
	for (int i = task->start; i < task->end; i++) {
		rect2 viewport = {
			t[i].position[0] - cam[i].size[0] / 2,
			t[i].position[1] - cam[i].size[1] / 2,
			t[i].position[0] + cam[i].size[0] / 2,
			t[i].position[1] + cam[i].size[1] / 2 };

		int count = space_cullaabb(&gfx->default_scene, viewport, cull_ids, CULLARRAY_SIZE);
		//TODO: sort array with renderinstance_t and chunk

		graphics_flush(gfx);
		glBindFramebuffer(GL_FRAMEBUFFER, cam->fbo);
		shader_use(resource_get(cam->shader));
		tranform_ortho(gfx->display_state.projection, viewport[0], viewport[2], viewport[1], viewport[3]);
		graphics_clear(gfx);
		
		for (int j = 0; j < count; j++) {
			entity_t ent = cull_ids[j];
			ENTITY_GETCOMP(task->wld, Transform, t, ent);
			ENTITY_GETCOMP(task->wld, Sprite, spr, ent);
			ENTITY_GETCOMP(task->wld, Tiles, tiles, ent);
			mat3 transform;
			transform_set(transform,
				t->position[0] - spr->offset[0],
				t->position[1] - spr->offset[1],
				0.0f, spr->size[0], spr->size[1],
				0.0f, 0.0f, 0.0f, 0.0f);

			if (tiles) {
				graphics_draw_tiles(gfx, spr->texture, transform,
					tiles->data, TILE_ROWS, TILE_COLS,
					spr->color, spr->alpha);
			}
			else {
				graphics_draw_quad(gfx, spr->texture, transform,
					spr->index, spr->color, spr->alpha);
			}
		}
		graphics_flush(gfx);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	shader_use(resource_get(gfx->default_shader));
	tranform_ortho(gfx->display_state.projection, 0, gfx->display_state.width, gfx->display_state.height, 0);
	return 0;
}
