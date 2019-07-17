#include "async.h"
#include "utils.h"
#include "window.h"
#include "platform.h"

static void timer_cb(uv_timer_t* handle);

//Timer.new(timeout, repeat, callback)
static int timer_new(lua_State* L) {
	Timer *timer;
	double timeout = luaL_checknumber(L, 2);
	double repeat = luaL_checknumber(L, 3);
	int cb = luaL_ref(L, LUA_REGISTRYINDEX);

	timer = (Timer*)lua_newuserdata(L, sizeof(Timer));
	luaL_setmetatable(L, Timer_mt);
	timer->callstate = L;
	timer->callback = cb;
	timer->lasttime = get_time();
	uv_timer_init(uv_default_loop(), (uv_timer_t*)timer);
	uv_timer_start((uv_timer_t*)timer, timer_cb, (int)(timeout * 1000), (int)(repeat * 1000));
	return 1;
}
//Timer.__gc()
static int timer_delete(lua_State* L) {
	Timer* timer = luaL_checkudata(L, 1, Timer_mt);
	luaL_unref(L, LUA_REGISTRYINDEX, timer->callback);
	uv_timer_stop(timer);
	return 0;
}
//Timer callback(dt)
static void timer_cb(uv_timer_t* handle) {
	Timer* timer = (Timer*)handle;
	double current = get_time();
	lua_rawgeti(timer->callstate, LUA_REGISTRYINDEX, timer->callback);
	if (lua_isfunction(timer->callstate, -1)) {
		lua_pushnumber(timer->callstate, current - timer->lasttime);
		lua_call(timer->callstate, 1, 0);
	}
	timer->lasttime = current;
}

#ifdef DEBUG
/* Console code assumes a Console lua object with "hist" element */
static char* console_source = //TODO: make console scrollable
"Console = {active = false, hist = '', start_token = '\\144> ', max_lines = 10, offset_line = 0 }"
"function Console:new()"
"	local s = {}"
"	s.width = Config.width / Config.zoom"
"	s.height = Config.height / Config.zoom"
"	s.line = Console.start_token"
"	self.active = true"

"	function s:text(c)"
"		if c == '`' then"
"			Console.active = false"
"			Scene:pop()"
"		elseif c == '\\r' then"
"			local byteoffset = utf8.offset(self.line, -1)"
"			if byteoffset > 3 then self.line = self.line:sub(1, byteoffset - 1) end"
"		elseif c == '\\n' then"
"			print(self.line)"
"			local chunk, message = load(self.line:sub(4))"
"			if chunk == nil then print('Syntax error: ' .. message)"
"			else"
"				local err, message = pcall(chunk)"
"				if not err then print('Error: ' .. message) end"
"			end"
"			self.line = Console.start_token"
"		else self.line = self.line .. c end"
"	end"
"	function s:input(button)"
"		if button == INPUT.UP then Console.offset_line = math.min(Console.offset_line + 1, #array(lines(Console.hist)) - Console.max_lines)"
"		elseif button == INPUT.DOWN then Console.offset_line = math.max(0, Console.offset_line - 1)"
"		end"
"	end"
"	function s:draw()"
"		local y = 0"
"		local hist_lines = array(lines(Console.hist))"
"		local start_line = #hist_lines - Console.max_lines - Console.offset_line"
"		for i, l in ipairs(hist_lines) do"
"			if i > start_line and i <= start_line + Console.max_lines then"
"				Sprite.quad(nil, 0, y, 0, self.width, 16, 0)"
"				Sprite.text(Resources['lucida'], l, 4, y + 4)"
"				y = y + 12"
"			end"
"		end"
"		Sprite.quad(nil, 0, y, 0, self.width, 16, 0)"
"		Sprite.text(Resources['lucida'], self.line, 4, y + 4)"
"	end"
"	return s end";

void console_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	lua_State* L = stream->data;
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
	window_poll();
	event_pump();
}

static int async_run(lua_State* L) {
#ifdef DEBUG
	if (luaL_dostring(L, console_source)) {
		printf("Error loading console script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	uv_pipe_t pipe1;
	pipe1.data = L;
	uv_pipe_init(uv_default_loop(), &pipe1, 0);
	uv_pipe_bind(&pipe1, pipe_name);
	uv_listen((uv_stream_t*)&pipe1, 1, console_on_freopen);
	FILE *fp = freopen(pipe_name, "w", stdout);
#endif
	uv_idle_t events;
	uv_idle_init(uv_default_loop(), &events);
	uv_idle_start(&events, event_poll_cb);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	return 0;
}

int async_stop() {
	uv_stop(uv_default_loop());
	window_close();
	return 0;
}

int openlib_Async(lua_State* L) {
	static luaL_Reg timer_lib[] = {
		{"__gc", timer_delete},
		{NULL, NULL}
	};
	create_lua_class(L, Timer_mt, timer_new, timer_lib);

	static luaL_Reg app_lib[] = {
		{"run", async_run},
		{"stop", async_stop},
		{NULL, NULL}
	};
	create_lua_lib(L, App_mt, app_lib);

	event_register(EVT_QUIT, async_stop, NULL);
	return 0;
}