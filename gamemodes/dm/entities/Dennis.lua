
local NUM_ITEMS = 6

-- tables defined at class level will be copied into each object created from this class
Dennis.myTable = {
	tbl={
		a=1,
		b=2
	},
	c=3
}

function Dennis.OnCreate()
	self.ents = {}
	print("ASDONASODJASJIODAJIOSDJIOASJIODJIOASJIOD", self.__dbgId, tostring(self.ents))
	for i = 1, NUM_ITEMS do
		--print("before", tostring(self))
		local ent = Srv.Game:CreateEntityPickup(i%2, 0) -- armor
		--print("after", tostring(self))
		self.ents[i] = ent
		print("num: " .. #self.ents)
	end
	print("Dennis alive", self.__dbgId, tostring(this))

	print(self.myTable.tbl.a, self.myTable.tbl.b, self.myTable.c)
	self.myTable.tbl.a, self.myTable.tbl.b, self.myTable.c = 10, 11, 12

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

	--print(tostring(self), tostring(this))

	this:Tick()

end

function Dennis.attachTo(self, parent)
	print("Dennis attach", self.__dbgId, tostring(this), tostring(parent))
	self.parent = parent
end
