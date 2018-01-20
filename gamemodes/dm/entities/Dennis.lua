
local NUM_ITEMS = 6

function Dennis.OnCreate()
	self.ents = {}
	print("ASDONASODJASJIODAJIOSDJIOASJIODJIOASJIOD", self.__dbgId, tostring(self.ents))
	for i = 1, NUM_ITEMS do
		print("before", tostring(self))
		local ent = Srv.Game:CreateEntityPickup(i%2, 0) -- armor
		print("after", tostring(self))
		self.ents[i] = ent
	end
	print("Dennis alive", self.__dbgId, tostring(this))
end

function Dennis.Tick()
	this:Tick()

	if self.parent == nil then return end

	-- update armor circle
	local Tick = Srv.Server.Tick
	local Pos = self.parent.m_Pos
	for i,ent in ipairs(self.ents) do
		local s = (i/#self.ents)*2*math.pi
		local x = Pos.x + 40*math.cos(Tick/30 + s)
		local y = Pos.y + 40*math.sin(Tick/30 + s)
		ent.m_Pos = vec2(x, y)
	end

	this:Tick()

end

function Dennis.attachTo(self, parent)
	print("Dennis attach", self.__dbgId, tostring(this), tostring(parent))
	self.parent = parent
end
