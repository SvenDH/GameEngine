#include "app.h"
#include "utils.h"

#define BUTTONS \
	X(no, 0) \
	X(space, GLFW_KEY_SPACE ) \
	X(right, GLFW_KEY_RIGHT) \
	X(left, GLFW_KEY_LEFT) \
	X(down, GLFW_KEY_DOWN) \
	X(up, GLFW_KEY_UP) \
	X(enter, GLFW_KEY_ENTER) \
	X(escape, GLFW_KEY_ESCAPE) \
	X(shift, GLFW_KEY_LEFT_SHIFT) \
	X(control, GLFW_KEY_LEFT_CONTROL) \
	X(alt, GLFW_KEY_LEFT_ALT) \
	X(backspace, GLFW_KEY_BACKSPACE) \
	X(tab, GLFW_KEY_TAB) \
	X(home, GLFW_KEY_HOME) \
	X(end, GLFW_KEY_END) \
	X(delete, GLFW_KEY_DELETE) \
	X(insert, GLFW_KEY_INSERT) \
	X(pageup, GLFW_KEY_PAGE_UP) \
	X(pagedown, GLFW_KEY_PAGE_DOWN) \
	X(pause, GLFW_KEY_PAUSE) \
	X(printscreen, GLFW_KEY_PRINT_SCREEN) \
	X(f1, GLFW_KEY_F1) \
	X(f2, GLFW_KEY_F2) \
	X(f3, GLFW_KEY_F3) \
	X(f4, GLFW_KEY_F4) \
	X(f5, GLFW_KEY_F5) \
	X(f6, GLFW_KEY_F6) \
	X(f7, GLFW_KEY_F7) \
	X(f8, GLFW_KEY_F8) \
	X(f9, GLFW_KEY_F9) \
	X(f10, GLFW_KEY_F10) \
	X(f11, GLFW_KEY_F11) \
	X(f12, GLFW_KEY_F12)

enum button_enum {
#define X(_enum, _val) key_##_enum = _val,
	BUTTONS
#undef X
	PRESS_BUTTON = 0x80,
	RELEASE_BUTTON = 0x40,
	REPEAT_BUTTON = 0x20
};

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
	if (!window->glfw)
		fprintf(stderr, "Error: Failed to create GLFW window");

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
	event_post((event_t) { .type = on_text, .p0.data = codepoint });
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			char k = 0;
			switch (key) {
			case GLFW_KEY_ENTER: k = '\n'; break;
			case GLFW_KEY_BACKSPACE: k = '\r'; break;
			case GLFW_KEY_TAB: k = '\t'; break;
			}
			if (k) event_post((event_t) { .type = on_text, .p0.data = k });
		}
		double time = get_time();
		if (action == GLFW_PRESS) {
			app_instance()->keys[key] = 1;
			event_post((event_t) { .type = on_press, .p0.data = key, .p1.nr = time });
		}
		else if (action == GLFW_RELEASE) {
			app_instance()->keys[key] = 0;
			event_post((event_t) { .type = on_release, .p0.data = key, .p1.nr = time });
		}
	}
}

static void close_callback(GLFWwindow* window) {
	event_post((event_t) { .type = on_quit });
}

//window.resize(width, height)
static int w_window_resize(lua_State* L) {
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);
	window_resize(window_instance(), width, height);
	return 0;
}

//window.title(title)
static int w_window_title(lua_State* L) {
	const char* title = luaL_checkstring(L, 1);
	window_title(window_instance(), title);
	return 0;
}

//input.get(key)
static int w_input_get(lua_State* L) {
	int i;
	char* keys = &app_instance()->keys;
	const char* str;
	size_t len;
	switch (lua_type(L, 1)) {
	case LUA_TNUMBER:
		i = lua_tointeger(L, 1);
		lua_pushboolean(L, keys[i]);
		break;
	case LUA_TSTRING:
		str = lua_tolstring(L, 1, &len);
		if (len == 1) {
			i = str[0];
		}
		else {
#define X(_enum, _val) \
			if (len == sizeof(#_enum)-1 && !memcmp(str, #_enum, sizeof(#_enum)-1)) { \
				i = _val; \
			} \
			else
	BUTTONS
#undef X
			i = 0;
		}
		lua_pushboolean(L, keys[i]);
		break;
	}
	return 1;
}

int openlib_Window(lua_State* L) {
	static luaL_Reg win_lib[] = {
		{"resize", w_window_resize},
		{"title", w_window_title},
		{NULL, NULL}
	};
	create_lua_lib(L, Window_mt, win_lib);

	lua_newtable(L);
#define X(_enum, _name) \
	lua_pushliteral(L, #_enum); \
	lua_pushnumber(L, _name); \
	lua_settable(L, -3);
	BUTTONS
#undef X
	lua_pushliteral(L, "release"); lua_pushnumber(L, RELEASE_BUTTON); lua_settable(L, -3);
	lua_pushliteral(L, "press"); lua_pushnumber(L, PRESS_BUTTON); lua_settable(L, -3);
	lua_setglobal(L, "keys");

	static luaL_Reg inp_lib[] = {
		{"get", w_input_get},
		{NULL, NULL}
	};
	create_lua_lib(L, Input_mt, inp_lib);

	return 0;
}
