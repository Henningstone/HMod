
local NUM_RINGS = 3
local NUM_ITEMS = 8
local OFFSET_BASE = 64
local OFFSET_MOVE = 64*2
local MAX_CAPTURE_DIST = 500

-- tables defined at class level will be copied into each object created from this class
Dennis.myTable = {
	tbl={
		a=1,
		b=2
	},
	c=3
}

Dennis.parent = nil
Dennis.active = true
Dennis.ents = {}
Dennis.captured = {}
Dennis.lastFire = 0

function Dennis.OnCreate()
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

local function findClosestPlayer(Pos)
	local ClosestID = -1
	local ClosestDist = 9999
	local chr
	for CID = 0, 15 do
		local pl = Srv.Game:GetPlayer(CID)
		print(CID, 'pl', tostring(pl))
		if pl ~= nil then
			chr = pl:GetCharacter()
			print(CID, tostring(chr))
		end

		-- check if we've got someone and he's not ourselves
		if chr ~= nil and CID ~= self.parent:GetPlayer():GetCID() then
			-- make sure he's also not yet captured by us
			local gotHim = false
			for _,v in ipairs(self.captured) do
				if v == CID then
					gotHim = true
					break
				end
			end
			print(CID, ClosestID, ClosestDist, tostring(gotHim))
			if not gotHim then
				local ThisDist = Srv.Game.Collision:Distance(this.Pos, chr.Pos)
				if ClosestID == -1 or ThisDist < ClosestDist then
					print("Found new closest player " .. Srv.Server:GetClientName(CID))
					ClosestID = CID
					ClosestDist = ThisDist
				end
			end
		end
	end

	return ClosestID, chr
end

local function releaseFirst()
	if #self.captured > 0 then
		local chr = Srv.Game:GetPlayerChar(self.captured[1])
		table.remove(self.captured, 1)
		chr:GetSelf().myDennis:GetSelf().active = true
	else
		Srv.Game:SendChatTarget(self.parent:GetPlayer():GetCID(), "No tee's captured, use your hammer first.")
	end
end

local function capturePlayer()
	if #self.captured < NUM_RINGS then
		local ClosestID, chr = findClosestPlayer(this.Pos)
		if ClosestID < 0 then
			Srv.Game:SendChatTarget(self.parent:GetPlayer():GetCID(), "No Tee found in range of " .. MAX_CAPTURE_DIST)
		else
			-- success
			table.insert(self.captured, ClosestID)
			chr:GetSelf().myDennis:GetSelf().active = false
			print(chr:GetSelf().myDennis:GetSelf().active, "assssssssssssssssssssssss")
		end
	else
		Srv.Game:SendChatTarget(self.parent:GetPlayer():GetCID(), "All slots full, use your gun to release.")
	end
end

local function checkInput()
	local input = self.parent.Core.Input
	--print("input", input.Fire, input.WantedWeapon)

	local fire = math.ceil(input.Fire/2)
	if fire > self.lastFire then
		print(fire, self.lastFire, input.WantedWeapon)
		self.lastFire = fire
		if input.WantedWeapon == 1 then  -- hammer; capture
			capturePlayer()
		elseif input.WantedWeapon == 2 then  -- gun; release
			releaseFirst()
		end
	end
end


local function updateVictimPositions(ringPoses)
	for i,v in ipairs(self.captured) do
		local chr = Srv.Game:GetPlayerChar(v)
		if chr ~= nil then
			chr.Pos = ringPoses[i]
			chr.Core.Pos = ringPoses[i]
			chr.Core.Vel = vec2(0,0)
		end
	end
end


function Dennis.Tick()
	--this:Tick()

	if self.parent == nil or self.active == false then
		for ring = 1, NUM_RINGS do
			for i,ent in ipairs(self.ents[ring]) do
				ent.Pos = vec2(0, 0)
				this:Tick()
			end
		end
		return
	end

	local ringPoses = {}

	for ring = 1, NUM_RINGS do
		local ro = (ring/NUM_RINGS) * 2*math.pi

		-- update armor circle
		local Time = Srv.Server.Tick/30
		local Pos = self.parent.Pos
		for i,ent in ipairs(self.ents[ring]) do
			local s = (i/#self.ents[ring])*2*math.pi
			local x = Pos.x + OFFSET_BASE*math.cos(Time + s) + (OFFSET_MOVE*math.cos(Time + ro))
			local y = Pos.y + OFFSET_BASE*math.sin(Time + s) + (OFFSET_MOVE*math.sin(Time + ro))

			-- add all the positions together so we can later take an average (that's where the tee will go)
			if ringPoses[ring] == nil then
				ringPoses[ring] = vec2(x, y)
			else
				ringPoses[ring] = ringPoses[ring] + vec2(x, y)
			end

	--		local solid = Srv.Game.Collision:IsTileSolid(x, y)
			--print(tostring(solid))
	--		if not solid then
				ent.Pos = vec2(x, y)
	--		end
		end

		-- get the average
		ringPoses[ring] = ringPoses[ring] / #self.ents[ring]

	end

	--print(tostring(self), tostring(this))

	checkInput()
	updateVictimPositions(ringPoses)

	this:Tick()
end


function Dennis.attachTo(self, parent) -- parent is CCharacter
	print("Dennis attach", self.__dbgId, tostring(this), tostring(parent), type(parent))
	self.parent = parent
end


