image_dir = '../resources/images/'
script_dir = '../resources/scripts/'
shader_dir = '../resources/shaders/'
sprite_dir = '../resources/sprites/'
sound_dir = '../resources/sounds/'

return {
	font = {
		default = texture{ image = image(image_dir .. 'lucida.png'), width = 8, height = 12 },
		courier = texture{ image = image(image_dir .. 'courier.png'), width = 8, height = 16 },
	},
	texture = {
		space = texture{ image = image(image_dir .. 'space.png'), width = 640, height = 480 },
		menu = texture{ image = image(image_dir .. 'menu.png'), width = 200, height = 160 },
		menuitem = texture{ image = image(image_dir .. 'menuitem.png'), width = 200, height = 40 },
		dungeon = texture{ image = image(image_dir .. 'dungeon.png'), width = 16, height = 16 },
		castle = texture{ image = image(image_dir .. 'castle.png'), width = 16, height = 16 },
		knight = texture{ image = image(image_dir .. 'knight.png'), width = 32, height = 32 },
	},
	script = {
		startscene = script(script_dir .. 'startscene.lua'),
		menuscene = script(script_dir .. 'menuscene.lua'),
		optionscene = script(script_dir .. 'optionscene.lua'),
		gamescene = script(script_dir .. 'gamescene.lua'),
		menuscript = script(script_dir .. 'menu.lua'),
	},
	sound = {
		test = sound(sound_dir .. 'test.wav')
	}
}