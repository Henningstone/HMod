
local OFFSET_BASE = 64
local OFFSET_MOVE = 64*2

Dennis.NUM_RINGS = 3
Dennis.NUM_ITEMS = 8
Dennis.MAX_CAPTURE_DIST = 500

Dennis.parent = nil  -- CPlayer that we belong to
Dennis.active = false  -- whether the rings are there
Dennis.ents = {}  -- 2-dimensional table to store all entites by [ringID][index]
Dennis.captured = {}  -- contains the CIDs of player's we've captured
Dennis.fired = 0

function Dennis.OnInsert()
	print("ASDONASODJASJIODAJIOSDJIOASJIODJIOASJIOD Dennis.OnCreate", self.__dbgId, tostring(self.ents))

	-- allocate all the entities, but hide them from the world
	for ring = 1, self.NUM_RINGS do
		self.ents[ring] = {}

		for i = 1, self.NUM_ITEMS do
			-- create an entity
			local ent = Srv.Game:CreateEntityPickup(i%2, 0) -- armor and hearts
			ent:GetSelf().CanNotBePickedUp = true

			-- insert it
			self.ents[ring][i] = ent

			-- hide it from the world (this does not destroy it, the reference will still be valid!)
			Srv.Game.World:RemoveEntity(ent)

			print("num: " .. #self.ents .. "-" .. #self.ents[ring])
		end
	end
	print("Dennis alive", self.__dbgId, tostring(this))

end

-- called from `Player.enableDennis' via `Character.OnCreate'
function Dennis.spawn(self)
	if self.active then
		print("Dennis.spawn called although dennis active?")
		return false
	end

	self.active = true

	for ring,tr in ipairs(self.ents) do
		for i,ent in ipairs(tr) do
			-- hide it from the world
			Srv.Game.World:InsertEntity(ent)
		end
	end
	print("Dennis shown", self.__dbgId, tostring(this))

	return true
end

-- called from `Player.disableDennis' via `Character.Destroy'
function Dennis.disappear(self)
	if not self.active then
		print("Dennis.disappear called although dennis inactive?")
		return false
	end

	for ring,tr in ipairs(self.ents) do
		for i,ent in ipairs(tr) do
			-- hide it from the world
			Srv.Game.World:RemoveEntity(ent)
		end
	end
	print("Dennis hidden", self.__dbgId, tostring(this))

	self.active = false
	return true
end

local function ReleaseNextVictim()
	print("ReleaseNextVictim")
	if #self.captured > 0 then
		local CID = self.captured[1]
		local pl = Srv.Game:GetPlayer(CID)
		table.remove(self.captured, 1)
		pl:GetSelf():enableDennis(true)
		pl:GetCharacter().PhysicsEnabled = true
		Srv.Game:SendChatTarget(self.parent:GetCID(), Srv.Server:GetClientName(CID) .. " freed.")
	else
		Srv.Game:SendChatTarget(self.parent:GetCID(), "No tee's captured, use your gun first.")
	end
end

function Dennis.Destroy()
	print("XNKLXNMXBMNVHJBCJBVJBJKBJKVBVJH Dennis.Destroy", self.__dbgId, tostring(self.ents))

	-- release all
	for _= 1, #self.captured do
		ReleaseNextVictim()
	end

	for ring,tr in ipairs(self.ents) do
		for i,ent in ipairs(tr) do
			Srv.Game.World:DestroyEntity(ent)
		end
	end

	self.ents = nil
end

local function FindClosestPlayer(Pos)
	local ClosestID = -1
	local ClosestDist = 9999
	for CID = 0, 15 do
		local chr
		local pl = Srv.Game:GetPlayer(CID)
		print(CID, 'pl', tostring(pl))
		if pl ~= nil then
			chr = pl:GetCharacter()
			--print(CID, tostring(chr))
		end

		-- check if we've got someone and he's not ourselves
		if chr ~= nil and CID ~= self.parent:GetCID() then
			-- make sure he's also not yet captured by us
			local gotHim = false
			for _,v in ipairs(self.captured) do
				if v == CID then
					gotHim = true
					print("already got " .. v)
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

	local Chr
	if ClosestID > -1 then
		Chr = Srv.Game:GetPlayer(ClosestID):GetCharacter()
	end
	return ClosestID, Chr
end

local function CapturePlayer()
	print("CapturePlayer")
	if #self.captured < self.NUM_RINGS then
		local ClosestID, chr = FindClosestPlayer(this.Pos)
		if ClosestID < 0 then
			Srv.Game:SendChatTarget(self.parent:GetCID(), "No Tee found in range of " .. self.MAX_CAPTURE_DIST)
		else
			-- success
			table.insert(self.captured, ClosestID)
			chr:GetPlayer():GetSelf():disableDennis(true)
			chr.PhysicsEnabled = false
			Srv.Game:SendChatTarget(self.parent:GetCID(), "Captured " .. Srv.Server:GetClientName(ClosestID) .. " into slot " .. #self.captured .. "/" .. self.NUM_RINGS)
		end
	else
		Srv.Game:SendChatTarget(self.parent:GetCID(), "All slots full, use your hammer to release.")
	end
end

local function CheckInput()
	if self.fired == 0 then return end
	print("CheckInput; fired", self.fired)

	if self.fired == 1 then  -- hammer; capture
		ReleaseNextVictim()
	elseif self.fired == 2 then  -- gun; release
		CapturePlayer()
	end

	self.fired = 0
end


local function UpdateVictimPositions(ringPoses)
	for i,v in ipairs(self.captured) do
		local chr = Srv.Game:GetPlayerChar(v)
		if chr ~= nil then
			local core = chr.Core
			local WantedPos = ringPoses[i]
			if Srv.Game.Collision:Distance(core.Pos, WantedPos) > 21 then
				-- smooth flying transition
				core.Vel = Srv.Game.Collision:Normalize(WantedPos - core.Pos) * 16
				core.Pos = core.Pos + core.Vel
			else
				-- rotate him
				core.Pos = WantedPos

				local RadialOut = Srv.Game.Collision:Normalize(core.Pos - this.Pos)
				local CircleDir = Srv.Game.Collision:Rotate(RadialOut, 90)
				CircleDir = CircleDir * 4
				--print(tostring(CircleDir))
				core.Vel = CircleDir + self.parent:GetCharacter().Core.Vel
			end

			chr.Pos = core.Pos
		end
	end
end


function Dennis.trigger(self, Weapon)
	if Weapon == 1 or Weapon == 2 then
		self.fired = Weapon
	end
end


function Dennis.Tick()

	if self.parent == nil or self.active == false then
		--print("Tick prevented", tostring(self.parent), tostring(self.active))
		return
	end

	--if self.parent:GetCharacter() == nil then return end  -- TODO XXX THIS SHOULD NOT HAPPEN

	local ringPoses = {}

	for ring = 1, self.NUM_RINGS do
		local ro = (ring/self.NUM_RINGS) * 2*math.pi

		-- update armor circle
		local Time = Srv.Server.Tick/30
		local Pos = self.parent:GetCharacter().Pos
		this.Pos = Pos
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

	CheckInput()
	UpdateVictimPositions(ringPoses)

	this:Tick()
end

-- called from Player.createDennis
function Dennis.attachTo(self, parent) -- parent is CPlayer
	self.parent = parent
	print("Dennis attach", self.__dbgId, tostring(this), tostring(parent), type(parent))
end


