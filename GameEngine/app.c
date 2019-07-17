#include "platform.h"
#include "async.h"
#include "sprite.h"
#include "texture.h"
#include "shader.h"
#include "graphics.h"
#include "event.h"
#include "window.h"
#include "file.h"
#include "ecs.h"
#include "math.h"

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static lua_State *L;

static char* boot_source =
"function split(path, sep) sep=sep or '%s' local t = {} local i = 1 for w in string.gmatch(path, '([^'..sep..']+)') do t[i] = w i = i + 1 end return t end\n"
"function lines(s) if s:sub(-1) ~= '\\n' then s = s..'\\n' end return s:gmatch('(.-)\\n') end\n"
"function hex_dump(buf) for i = 1, math.ceil(#buf / 16) * 16 do io.write(i > #buf and '   ' or string.format('%02X ', buf:byte(i))) if i % 8 == 0 then io.write(' ') end if i % 16 == 0 then io.write('\\n') end end io.flush() end\n"
"function array(...) local arr = {} for v in ... do arr[#arr + 1] = v end return arr end\n"

"Resources = {}"
"local script_dir = '../resources/scripts'"
"local shader_dir = '../resources/shaders'"
"local sprite_dir = '../resources/sprites'"

"for path in Dir(script_dir) do"
"	opts = split(path, '.')"
"	f = File(script_dir .. '/' .. path)"
"   Resources[opts[1]] = assert(load(f:read()))()"
"	f:close()"
"end\n"

"Config = Resources['Config']"
"Window.init(Config.title, Config.width, Config.height)"
"Graphics.init(Config.width, Config.height, Config.zoom)"

"for path in Dir(shader_dir) do"
"	opts = split(path, '.')"
"	f = File(shader_dir .. '/' .. path)"
"   Resources[opts[1]] = Shader(f:read())"
"	f:close()"
"end\n"

"for path in Dir(sprite_dir) do"
"	opts = split(path, '.')"
"	f = File(sprite_dir .. '/' .. path)"
"   Resources[opts[1]] = Texture(f:read(), opts[2], opts[3])"
"	f:close()"
"end\n"

"Entry = Resources[Config['entry']]"
"Scene:push(Entry:new())"

"timer = Timer(0, 1/Config.fps, function(dt)"
"	Event.post(EVENT.UPDATE, dt)"
"	Event.post(EVENT.DRAW)"
"end)\n"

"App.run()";

int main(int argc, const char* argv[]) {
	//Init lua
	L = luaL_newstate();

	luaL_openlibs(L);
	openlib_Math(L);
	openlib_Window(L);
	openlib_File(L);
	openlib_Graphics(L);
	openlib_Sprite(L);
	openlib_Shader(L);
	openlib_Texture(L);
	openlib_Event(L);
	openlib_Async(L);
	openlib_ECS(L);

	if (luaL_dostring(L, boot_source)) {
		printf("Error loading boot script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	return 0;
}
