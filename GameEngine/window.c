#include "window.h"
#include "input.h"
#include "async.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

GLFWwindow* window;

//Window callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void character_callback(GLFWwindow* window, unsigned int codepoint);
void close_callback(GLFWwindow* window);

//Window.init(title, width, height)
int window_init(lua_State* L) {
	char* title = luaL_checkstring(L, 1);
	int width = luaL_checkinteger(L, 2);
	int height = luaL_checkinteger(L, 3);
	if (title == NULL || strlen(title) == 0) title = "Untitled";
	if (width < 1 ) width = 1; 
	if (height < 1) height = 1;

	if (!glfwInit())
		exit(1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to create GLFW window");
		exit(1);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, character_callback);
	glfwSetWindowCloseCallback(window, close_callback);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Failed to initialize GLAD");
		exit(1);
	}
	return 0;
}

//Window.resize(width, height)
int window_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	glfwSetWindowSize(window, width, height);
	return 0;
}

//Window.title(title)
int window_title(lua_State* L) {
	char* title = luaL_checkstring(L, 1);
	glfwSetWindowTitle(window, title);
	return 0;
}

//Window.swap()
int window_swap(lua_State* L) {
	glfwSwapBuffers(window);
	return 0;
}

//Window.poll()
int window_poll(lua_State* L) {
	glfwPollEvents();
	return 0;
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
	text_enqueue(codepoint);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	static int input_map[] = { 0, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_ESCAPE, GLFW_KEY_TAB, NULL };
	if (action == GLFW_PRESS) {
		for (int i = 1; input_map[i]; i++) {
			if (key == input_map[i]) {
				double time = get_time();
				input_enqueue(i, time);
				break;
			}
		} //No mapped keys input special text
		switch (key) {
		case GLFW_KEY_ENTER: text_enqueue('\n'); return;
		case GLFW_KEY_BACKSPACE: text_enqueue('\r'); return;
		case GLFW_KEY_TAB: text_enqueue('\t'); return;
		}
	}
}

void close_callback(GLFWwindow* window) {
	async_stop();
}

int openlib_Window(lua_State* L) {
	lua_settop(L, 0);
	static luaL_Reg lib[] = {
		{"init", window_init},
		{"resize", window_resize},
		{"title", window_title},
		{"swap", window_swap},
		{"poll", window_poll},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	lua_setglobal(L, Window_mt);
	return 1;
}
