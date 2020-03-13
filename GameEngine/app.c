#include "app.h"
#include "utils.h"
#include "graphics.h"

static lua_State* L;

void app_init(app_t* manager, const char* name) {
	manager->name = name;
	manager->is_running = 0;
	manager->next_game_tick = 0;
	memset(manager->keys, 0, 1024);
	event_register(eventhandler_instance(), manager, quit, app_stop, NULL);
}

#ifdef DEBUG
/* Console code assumes a Console lua object with "hist" element */
void console_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	lua_getglobal(L, "Console");
	lua_pushstring(L, "hist");
	lua_pushvalue(L, -1);
	lua_gettable(L, -3);
	lua_pushlstring(L, buf->base, nread);
	lua_concat(L, 2);
	lua_settable(L, -3);
	free(buf->base);
}

void console_alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	*buf = uv_buf_init((char*)malloc(suggested_size), suggested_size);
}

void console_on_freopen(uv_stream_t* server, int status) {
	lua_State* L = server->data;
	uv_pipe_t* pipe2 = malloc(sizeof(uv_pipe_t));
	pipe2->data = L;
	uv_pipe_init(uv_default_loop(), pipe2, 0);
	uv_accept(server, (uv_stream_t*)pipe2);
	uv_read_start((uv_stream_t*)pipe2, console_alloc_buffer, console_on_read);
}
#endif

void event_poll_cb(uv_idle_t* handle) {
	app_t* manager = handle->data;
	eventhandler_t* handler = eventhandler_instance();
	window_poll(window_instance());
	event_pump(handler);
	if (manager->is_running) {
		double interpolation = 1.0 / SKIP_TICKS;
		double dt = get_time() - manager->next_game_tick;
		int loops = 0;
		while (get_time() > manager->next_game_tick && loops < MAX_FRAMESKIP) {
			//TODO: make stage objects/functions
			event_post(handler, (event_t) { .type = on_preupdate, .p0.nr = interpolation });
			event_pump(handler);
			event_post(handler, (event_t) { .type = on_update, .p0.nr = interpolation });
			event_pump(handler);
			event_post(handler, (event_t) { .type = on_postupdate, .p0.nr = interpolation });
			event_pump(handler);
			manager->next_game_tick += SKIP_TICKS;
			loops++;
		}
		interpolation = (get_time() + SKIP_TICKS - manager->next_game_tick) / SKIP_TICKS;
		
		graphics_clear(graphics_instance());

		event_post(handler, (event_t) { .type = on_predraw, .p0.nr = dt });
		event_pump(handler);
		event_post(handler, (event_t) { .type = on_draw, .p0.nr = dt });
		event_pump(handler);
		event_post(handler, (event_t) { .type = on_postdraw, .p0.nr = dt });
		event_pump(handler);

		event_post(handler, (event_t) { .type = on_pregui, .p0.nr = dt });
		event_pump(handler);
		event_post(handler, (event_t) { .type = on_gui, .p0.nr = dt });
		event_pump(handler);
		event_post(handler, (event_t) { .type = on_postgui, .p0.nr = dt });
		event_pump(handler);

		graphics_present(graphics_instance());

		window_swap(window_instance());
	}
}

int app_run(app_t* app) {
#ifdef DEBUG
	uv_pipe_t pipe1;
	uv_pipe_init(uv_default_loop(), &pipe1, 0);
	uv_pipe_bind(&pipe1, pipe_name);
	uv_listen((uv_stream_t*)&pipe1, 1, console_on_freopen);
	//FILE *fp = freopen(pipe_name, "w", stdout);
#endif
	uv_idle_t events;
	events.data = app;
	uv_idle_init(uv_default_loop(), &events);
	uv_idle_start(&events, event_poll_cb);

	app->is_running = 1;
	app->next_game_tick = (int)get_time();

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

int app_stop(app_t* app, event_t evt) {
	uv_stop(uv_default_loop());
	window_close(window_instance());
	app->is_running = 0;
	return 0;
}

static int w_app_run(lua_State* L) {
	app_run(app_instance());
	return 0;
}

static int w_app_stop(lua_State* L) {
	event_dispatch(eventhandler_instance(), (event_t) { .type = on_quit });
	return 0;
}

int openlib_App(lua_State* Lua) {
	L = Lua;
	static luaL_Reg app_lib[] = {
		{"run", w_app_run},
		{"stop", w_app_stop},
		{NULL, NULL}
	};
	create_lua_lib(Lua, App_mt, app_lib);	
	return 0;
}