local s = {}
s.width = config.width / config.zoom
s.height = config.height / config.zoom
local buttons =  {{text = "Back", func = function() scenes:pop() end}}
s.menu = Menu(buttons, 0, 0, s.width, s.height, resources.texture.menu, resources.texture.menuitem, resources.font.courier)

function s:input(button)
	self.menu:update(button)
end

function s:draw()
	self.menu:draw()
end

return s