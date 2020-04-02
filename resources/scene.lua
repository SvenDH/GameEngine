local Scene = {_scenes = {}}

function lines(s) if s:sub(-1) ~= '\n' then s = s..'\n' end return s:gmatch('(.-)\n') end

function Scene:push(...)
	if ... then
		local targs = {...}
		for _,v in ipairs(targs) do
			table.insert(self._scenes, v)
		end
    end
end

function Scene:pop(num)
    local num = num or 1
    local entries = {}

    for i = 1, num do
		if #self._scenes ~= 0 then
			table.insert(entries, self._scenes[#self._scenes])
			table.remove(self._scenes)
		else
			break
		end
    end
    return table.unpack(entries)
end

function Scene:clear()
	self._scenes = {}
end

function Scene:iter(reverse)
    local i = 0
	local n = #self._scenes

	return function ()
				i = i + 1
				if i <= n then return self._scenes[reverse and (n - (i-1)) or i] end
			end
 end

function Scene:text(evt, c)
	c = string.char(c)
	if Console and c == '`' and not Console.active then
		self:push(console:new())
	else
		for scene in self:iter(true) do
			if scene.text then
				scene:text(c)
				break
			end
		end
	end
end

function Scene:input(evt, button)
	for scene in self:iter(true) do
		if scene.input then
			scene:input(button)
			break
		end
	end
end

function Scene:update(evt, dt)
	for scene in self:iter(true) do
		if scene.update then scene:update(dt) end
	end

	if #self._scenes == 0 then Event.post(Event(EVENT.QUIT)) end
end

function Scene:draw(evt)
	for scene in self:iter() do
		if scene.draw then scene:draw() end
	end
end

event.listen(Scene, event.on_press, Scene.input)
event.listen(Scene, event.on_text, Scene.text)
event.listen(Scene, event.on_update, Scene.update)
event.listen(Scene, event.on_gui, Scene.draw)

return Scene