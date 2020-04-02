#include "platform.h"
#include "app.h"
#include "graphics.h"
#include "physics.h"
#include "event.h"
#include "file.h"
#include "resource.h"
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

"Console = {active = false, hist = '', start_token = '\\144> ', max_lines = 10, offset_line = 0 }"

"resources = script('../resources/resources.lua'):new()"
"config = script('../resources/config.lua'):new()"
"console = script('../resources/console.lua')"
"scenes = script('../resources/scene.lua'):new()"

"window.title(config.title)"
"window.resize(config.width, config.height)"
"graphics.resize(config.width, config.height)"

"Menu = resources.script.menuscript:new()"

"scenes:push(config.entry:new())"

"app.run()";

int main(int argc, const char* argv[]) {
	//Init lua
	L = luaL_newstate();

	luaL_openlibs(L);
	openlib_ECS(L);
	openlib_Math(L);
	openlib_Event(L);
	openlib_File(L);
	openlib_Resources(L);
	openlib_Window(L);
	openlib_Graphics(L);
	openlib_Physics(L);
	openlib_App(L);

	if (luaL_dostring(L, boot_source)) {
		printf("Error loading boot script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	return 0;
}
