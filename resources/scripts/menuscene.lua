local s = {}
s.width = config.width
s.height = config.height
local buttons = {{text = "New game", func = function() scenes:clear() scenes:push(resources.script.gamescene:new()) end},
				{text = "Join game", func = function() scenes:pop() end},
				{text = "Options", func = function() scenes:push(resources.script.optionscene:new()) end},
				{text = "Exit", func = function() scenes:clear() end}}
s.menu = Menu(buttons, 0, 0, s.width, s.height, resources.texture.menu, resources.texture.menuitem, resources.font.courier)

function s:input(button)
	self.menu:update(button)
end

function s:draw()
	self.menu:draw()
end

return s