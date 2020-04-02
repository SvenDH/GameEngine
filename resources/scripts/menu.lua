local Menu = {}
Menu.__index = Menu
setmetatable(Menu, { __call = function (cls, ...) return cls.new(...) end })

function Menu.new(buttons, x, y, w, h, background, menuitem, font)
	local m = setmetatable({}, Menu)
	m.x = x
	m.y = y
	m.width = w
	m.height = h
	m.background = background
	m.menuitem = menuitem
	m.font = font
	m.buttons = buttons
	m.nr = #buttons
	m.select = 1
	m.select_i = 0
	return m
end

function Menu:update(input)
	if input == string.byte('Q') then 
		self.select_i = 1
		self.buttons[self.select].func()
		self.select_i = 0
	elseif input == keys.up then
		if self.select == 1 then self.select = self.nr
		else self.select = self.select - 1 end
	elseif input == keys.down then
		if self.select == self.nr then self.select = 1
		else self.select = self.select + 1 end
	end
end

function Menu:draw()
	local index = 0
	local xpos = self.width/2 - self.background.width/2
	local ypos = self.height/2 - self.background.height/2
	self.background:draw(xpos, ypos)
	for i,button in ipairs(self.buttons) do
		if i==self.select then index = 1 + self.select_i
		else index = 0 end
		self.menuitem:draw(xpos, ypos, index)
		graphics.text(self.font, button.text, xpos + self.menuitem.width/2, ypos + self.menuitem.height/2 - 6, 1, 1)
		ypos = ypos + self.menuitem.height
	end
end

return Menu