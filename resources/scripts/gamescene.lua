local s = {}
s.width = config.width
s.height = config.height

resources.sound.test:load()

room0 = entity { 
	transform = { position = vector(0, 0) },
	sprite = { texture = resources.texture.dungeon, size = vector(16, 16) },
	collider = { size = vector(128, 128) },
	tiles = { data = {
		{16,16,16,16,16,16, 0, 0 },
		{ 8, 0, 0, 0, 0, 0, 0, 8 },
		{16, 0, 0, 0, 0, 0, 0,16 },
		{17, 0, 0, 0, 0, 0, 0,17 },
		{16, 0, 0, 0, 0, 0, 0, 0 },
		{17, 8,12, 0, 0,13, 8, 8 },
		{16,16,20, 8, 9,21,17,16 },
		{16,16,17,17,16,17,16,18 } } },
	
	}

room1 = entity { 
	transform = { position = vector(128, 0) },
	sprite = { texture = resources.texture.dungeon, size = vector(16, 16) },
	collider = { size = vector(128, 128) },
	tiles = { data = {
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 8, 0, 0, 0, 0, 0, 0, 8 },
		{16, 0, 0, 0, 0, 0, 0,16 },
		{17, 0, 0, 0, 0, 0, 0,17 },
		{ 0, 0, 0, 0, 0, 0, 0,18 },
		{12, 0, 0, 0, 0,13, 8,16 },
		{20, 8, 8, 8, 9,21,16,16 },
		{16,16,17,17,16,17,16,18 } } },
	}

player = entity {
	transform = { position = vector(64, 111) },
	sprite = { texture = resources.texture.knight, index = 0, offset = vector(16, 32), size = vector(32, 32), alpha = 1.0 },
	body = { speed = vector(0,0), force = vector(0,0), mass = 1.0, friction = 0.1, restitution = 0.0  },
	animation = { start_index = 8, end_index = 16, speed = 30 },
	collider = { offset = vector(8, 16), size = vector(16, 16) },
	camera = { size = vector(config.width / 2, config.height / 2) } }

box0 = entity {
	transform = { position = vector(24, 48) },
	sprite = { texture = resources.texture.dungeon, index = 16, offset = vector(8, 16), size = vector(16, 16), alpha = 1.0 },
	body = { speed = vector(0,0), force = vector(0,0), mass = 0.1, friction = 1.0, restitution = 0.0 },
	collider = { offset = vector(8, 16), size = vector(16, 16) }}

--room0.container.children = { player }


function s:input(button)
	
end

function s:update(dt)
	local force = vector(0,0)
	if input.get(keys.left) then
		force = force + vector(-1000,0)
	end
	if input.get(keys.right) then
		force = force + vector(1000,0)
	end
	if input.get("space") then
		force = force + vector(0,-1000)
	end

	player.body.force = force
end

function s:draw()
	
end

return s