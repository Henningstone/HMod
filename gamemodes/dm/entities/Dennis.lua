
local NUM_RINGS = 3
local NUM_ITEMS = 8
local OFFSET_BASE = 64
local OFFSET_MOVE = 64*2

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
	for ring = 1, NUM_RINGS do
		self.ents[ring] = {}
		for i = 1, NUM_ITEMS do
			--print("before", tostring(self))
			local ent = Srv.Game:CreateEntityPickup(i%2, 0) -- armor
			ent:GetSelf():setName("puu " .. i)
			--print("after", tostring(self))
			self.ents[ring][i] = ent
			print("num: " .. #self.ents)
		end
	end
	print("Dennis alive", self.__dbgId, tostring(this))

	print(self.myTable.tbl.a, self.myTable.tbl.b, self.myTable.c)
	self.myTable.tbl.a, self.myTable.tbl.b, self.myTable.c = 10, 11, 12

end

function Dennis.Tick()
	this:Tick()

	if self.parent == nil then return end

	for ring = 1, NUM_RINGS do
		local ro = (ring/NUM_RINGS) * 2*math.pi

		-- update armor circle
		local Time = Srv.Server.Tick/30
		local Pos = self.parent.Pos
		for i,ent in ipairs(self.ents[ring]) do
			local s = (i/#self.ents[ring])*2*math.pi
			local x = Pos.x + OFFSET_BASE*math.cos(Time + s) + (OFFSET_MOVE*math.cos(Time + ro))
			local y = Pos.y + OFFSET_BASE*math.sin(Time + s) + (OFFSET_MOVE*math.sin(Time + ro))
			ent.Pos = vec2(x, y)
		end
	end

	--print(tostring(self), tostring(this))

	this:Tick()

end

function Dennis.attachTo(self, parent)
	print("Dennis attach", self.__dbgId, tostring(this), tostring(parent))
	self.parent = parent
end
