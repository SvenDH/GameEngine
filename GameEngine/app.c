#include "async.h"
#include "scene.h"
#include "script.h"
#include "sprite.h"
#include "texture.h"
#include "shader.h"
#include "renderer.h"
#include "resources.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

//Lua state
static lua_State *L;

//Event parameters
Timer mainloop_timer;

//Display parameters
GLFWwindow* window;
static int width = 100;
static int height = 100;
static const char* title = "";
double deltaTime = 0.0;
double lastFrame = 0.0;

int state;

//Input parameters
const int input_map[] = { GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_ESCAPE, GLFW_KEY_TAB, NULL };

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	for (int i = 0; input_map[i]; i++) {
		if (key == input_map[i]) {
			double time = glfwGetTime();
			input_enqueue(i | (action == GLFW_PRESS) ? PRESS_BUTTON : RELEASE_BUTTON, time);
			break;
		}
	} //No mapped keys, special case?
	if (action) {
		switch (key) {
		case GLFW_KEY_ENTER: input_enqueue('\n', glfwGetTime()); return;
		case GLFW_KEY_BACKSPACE: input_enqueue('\r', glfwGetTime()); return;
		}
	}
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
	input_enqueue(codepoint, glfwGetTime());
}

void run() {
	if (!state) {
		lua_getglobal(L, "Config");
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, "title");
			title = luaL_checkstring(L, -1);
			lua_pop(L, 1);
			lua_getfield(L, -1, "width");
			width = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			lua_getfield(L, -1, "height");
			height = luaL_checkinteger(L, -1);
			lua_pop(L, 1);

			glfwSetWindowTitle(window, title);
			glfwSetWindowSize(window, width, height);

			lua_getfield(L, -1, "init");
			if (lua_isfunction(L, -1))
				lua_call(L, 0, 0);
			lua_pop(L, 2);

			state = 1;
		}
	}

	double currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	//Handle input
	glfwPollEvents();
	//Update scenes
	scenes_update(L, deltaTime);
	//Render scenes
	scenes_render(L, width, height);

	glfwSwapBuffers(window);
	if (glfwWindowShouldClose(window))
		uv_stop(uv_default_loop());
}

int main(int argc, const char* argv[]) {
	//Init program
	if (!glfwInit())
		exit(1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to create GLFW window");
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, character_callback);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Failed to initialize GLAD");
		exit(1);
	}

	//Init lua
	L = luaL_newstate();
	luaL_openlibs(L);
	openlib_Render(L);
	openlib_Resource(L);
	openlib_Script(L);
	openlib_Shader(L);
	openlib_Texture(L);
	openlib_Sprite(L);
	openlib_Input(L);
	openlib_Scene(L);

	timer_init(&mainloop_timer, run, 0, 17); //Run main loop 60 times per second

	//Run program
	async_run();

	//Close program
	//resources_clear();

	glfwTerminate();
	return 0;
}
