#include "async.h"
#include "scene.h"
#include "sprite.h"
#include "texture.h"
#include "shader.h"
#include "graphics.h"
#include "input.h"
#include "windows.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

//Lua state
static lua_State *L;

int main(int argc, const char* argv[]) {
	//Init lua
	L = luaL_newstate();
	luaL_openlibs(L);
	openlib_File(L);
	openlib_Window(L);
	openlib_Graphics(L);
	openlib_Sprite(L);
	openlib_Input(L);
	openlib_Scene(L);
	openlib_Shader(L);
	openlib_Texture(L);
	openlib_Timer(L);

	static char* boot_source =
		"function split(path, sep) sep=sep or '%s' local t = {} local i = 1 for w in string.gmatch(path, '([^'..sep..']+)') do t[i] = w i = i + 1 end return t end\n"

		"function boot()"
		"	console = False"
		"	Resources = {}"
		"	local script_dir = '../resources/scripts'"
		"	local shader_dir = '../resources/shaders'"
		"	local sprite_dir = '../resources/sprites'"

		"   for path in dir(script_dir) do"
		"		opts = split(path, '.')"
		"		f = file(script_dir .. '/' .. path)"
		"       Resources[opts[1]] = assert(load(f:read()))()"
		"		f:close()"
		"	end"

		"	Config = Resources['Config']"
		"	Window.init(Config.title, Config.width, Config.height)"
		"	Graphics.init(Config.width, Config.height, Config.zoom)"

		"	for path in dir(shader_dir) do"
		"		opts = split(path, '.')"
		"		f = file(shader_dir .. '/' .. path)"
		"       Resources[opts[1]] = Shader.load(f:read())"
		"		f:close()"
		"	end"

		"   for path in dir(sprite_dir) do"
		"		opts = split(path, '.')"
		"		f = file(sprite_dir .. '/' .. path)"
		"       Resources[opts[1]] = Texture.load(f:read(), opts[2], opts[3])"
		"		f:close()"
		"	end"

		"	timer = Timer.new(0, 1/Config.fps, function(dt)"
		"		Window.poll()"
		"		Scene.update(dt)"

		"		Graphics.clear()"
		"		Scene.render()"
		"		Graphics.present()"
		"		Window.swap()"
		"	end)"

		"	Entry = Resources[Config['entry']]"
		"	Scene.push(Entry:new(0, 0, Config.width / Config.zoom, Config.height / Config.zoom))"
		"end";

	luaL_dostring(L, boot_source);
	if (luaL_dostring(L, "boot()")) {
		printf("Error loading boot script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	//Run program
	async_run();

	glfwTerminate();
	return 0;
}
