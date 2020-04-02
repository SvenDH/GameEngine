local s = {}
s.width = config.width
s.height = config.height
s.bg = resources.texture.space

function s:input()
	scenes:push(resources.script.menuscene:new())
end

function s:draw()
	self.bg:draw(0, 0)
	graphics.text(resources.font.courier, "Press any button", self.width/2, self.height/2, 1, 0.8)
end

return s