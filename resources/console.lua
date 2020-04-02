local s = {}
s.width = config.width / config.zoom
s.height = config.height / config.zoom
s.line = Console.start_token
Console.active = true

function s:text(c)
	if c == '`' then
		Console.active = false
		scenes:pop()
	elseif c == '\r' then
		local byteoffset = utf8.offset(self.line, -1)
		if byteoffset > 3 then self.line = self.line:sub(1, byteoffset - 1) end
	elseif c == '\n' then
		print(self.line)
		local chunk, message = load(self.line:sub(4))
		if chunk == nil then print('Syntax error: ' .. message)
		else
			local err, message = pcall(chunk)
			if not err then print('Error: ' .. message) end
		end
		self.line = Console.start_token
	else self.line = self.line .. c end
end
function s:input(button)
	if button == keys.up then Console.offset_line = math.min(Console.offset_line + 1, #array(lines(Console.hist)) - Console.max_lines)
	elseif button == keys.down then Console.offset_line = math.max(0, Console.offset_line - 1)
	end
end
function s:draw()
	local y = 0
	local hist_lines = array(lines(Console.hist))
	local start_line = #hist_lines - Console.max_lines - Console.offset_line
	for i, l in ipairs(hist_lines) do
		if i > start_line and i <= start_line + Console.max_lines then
			graphics.quad(0, y, self.width, 16, 0)
			graphics.text(resources.font.default, l, 4, y + 4)
			y = y + 12
		end
	end
	graphics.quad(0, y, self.width, 16, 0)
	graphics.text(resources.font.default, self.line, 4, y + 4)
end

return s