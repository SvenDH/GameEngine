#include "window.h"
#include "event.h"
#include "async.h"

GLFWwindow* window;

static int input_map[] = { 0, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_ESCAPE, GLFW_KEY_TAB, NULL };

//Window callbacks
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void character_callback(GLFWwindow* window, unsigned int codepoint);
static void close_callback(GLFWwindow* window);

//Window.init(title, width, height)
static int window_init(lua_State* L) {
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
static int window_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	glfwSetWindowSize(window, width, height);
	return 0;
}

//Window.title(title)
static int window_title(lua_State* L) {
	char* title = luaL_checkstring(L, 1);
	glfwSetWindowTitle(window, title);
	return 0;
}

void window_swap() {
	glfwSwapBuffers(window);
}

void window_poll() {
	glfwPollEvents();
}

void window_close() {
	glfwDestroyWindow(window);
}

static void character_callback(GLFWwindow* window, unsigned int codepoint) {
	event_push(EVT_TEXT, &(char)codepoint, 1);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		for (char i = 1; input_map[i]; i++) {
			if (key == input_map[i]) {
				double time = get_time();
				event_push(EVT_INPUT, &i, 1);
				return;
			}
		} //No mapped keys input special text
		char k = 0;
		switch (key) {
		case GLFW_KEY_ENTER: k = '\n'; break;
		case GLFW_KEY_BACKSPACE: k = '\r'; break;
		case GLFW_KEY_TAB: k = '\t'; break;
		}
		if (k) event_push(EVT_TEXT, &k, 1);
	}
}

static void close_callback(GLFWwindow* window) {
	event_push(EVT_QUIT, NULL, 0);
}

static luaL_Reg lib[] = {
		{"init", window_init},
		{"resize", window_resize},
		{"title", window_title},
		{NULL, NULL}
};

#define E LUA_ENUM_HELPER
int openlib_Window(lua_State* L) {
	luaL_newlib(L, lib);
	lua_setglobal(L, Window_mt);

	lua_newtable(L);
	BUTTONS
		lua_pushliteral(L, "RELEASE"); lua_pushnumber(L, RELEASE_BUTTON); lua_settable(L, -3);
	lua_pushliteral(L, "PRESS"); lua_pushnumber(L, PRESS_BUTTON); lua_settable(L, -3);
	lua_setglobal(L, "INPUT");

	return 0;
}
#undef E
