#include "window.h"
#include "utils.h"

static int input_map[] = { 0, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_ESCAPE, GLFW_KEY_TAB, NULL };

//Window callbacks
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void character_callback(GLFWwindow* window, unsigned int codepoint);
static void close_callback(GLFWwindow* window);

void window_init(window_t* window, const char* name) {
	if (!glfwInit())
		exit(1);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window->glfw = glfwCreateWindow(640, 480, "", NULL, NULL);
	if (window->glfw == NULL)
		fprintf(stderr, "Failed to create GLFW window");

	glfwMakeContextCurrent(window->glfw);

	glfwSetKeyCallback(window->glfw, key_callback);
	glfwSetCharCallback(window->glfw, character_callback);
	glfwSetWindowCloseCallback(window->glfw, close_callback);

	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		fprintf(stderr, "Failed to initialize GLAD");
}

void window_resize(window_t* window, int width, int height) {
	window->width = width;
	window->height = height;
	glfwSetWindowSize(window->glfw, width, height);
}

void window_title(window_t* window, const char* title) {
	strcpy(window->title, title);
	glfwSetWindowTitle(window->glfw, title);
}

void window_swap(window_t* window) {
	glfwSwapBuffers(window->glfw);
}

void window_poll(window_t* window) {
	glfwPollEvents(window->glfw);
}

void window_close(window_t* window) {
	glfwDestroyWindow(window->glfw);
}

static void character_callback(GLFWwindow* window, unsigned int codepoint) {
	event_post(eventhandler_instance(), (Event) { .type = EVT_TEXT, .data = codepoint });
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		for (char i = 1; input_map[i]; i++) {
			if (key == input_map[i]) {
				double time = get_time();
				event_post(eventhandler_instance(), (Event) { .type = EVT_INPUT, .data = i });
				return;
			}
		} //No mapped keys input special text
		char k = 0;
		switch (key) {
		case GLFW_KEY_ENTER: k = '\n'; break;
		case GLFW_KEY_BACKSPACE: k = '\r'; break;
		case GLFW_KEY_TAB: k = '\t'; break;
		}
		if (k) event_post(eventhandler_instance(), (Event) { .type = EVT_TEXT, .data = k });
	}
}

static void close_callback(GLFWwindow* window) {
	event_post(eventhandler_instance(), (Event) { .type = EVT_QUIT, .data = 0 });
}

//Window.resize(width, height)
static int w_window_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	window_resize(window_instance(), width, height);
	return 0;
}

//Window.title(title)
static int w_window_title(lua_State* L) {
	const char* title = luaL_checkstring(L, 1);
	window_title(window_instance(), title);
	return 0;
}

int openlib_Window(lua_State* L) {
	static luaL_Reg lib[] = {
		{"resize", w_window_resize},
		{"title", w_window_title},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	lua_setglobal(L, Window_mt);

	lua_newtable(L);
#define X(name, luaname) \
	lua_pushstring(L, #luaname); \
	lua_pushnumber(L, name); \
	lua_settable(L, -3);
	BUTTONS
#undef X
	lua_pushliteral(L, "RELEASE"); lua_pushnumber(L, RELEASE_BUTTON); lua_settable(L, -3);
	lua_pushliteral(L, "PRESS"); lua_pushnumber(L, PRESS_BUTTON); lua_settable(L, -3);
	lua_setglobal(L, "INPUT");

	return 0;
}
