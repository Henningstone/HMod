
print("character loaded")

Character.myDennis = nil

function Character.OnCreate()
	print("Character", tostring(this), "has been created")
end

function Character.Tick()
	-- doubletime confirmed!!!11!1
	--this:Tick()
	this:Tick()

	--this:GetCore().Vel = this:GetCore().Vel - vec2(0, 1)
--[[
	this.Core.Input = this.Input
	this.Core.Tick(true)

	this.PrevInput = Input
	this:HandleWeapons()
	]]
end

function Character.spawnMyDennis(self, this)
	if self.myDennis ~= nil then
		print("Character", tostring(this), "has already got a dennis!!")
		return
	end

	print("Character", tostring(this), "gets his dennis")

	local dennis = Srv.Game:CreateEntityCustom("Dennis")
	dennis:GetSelf():attachTo(this)  -- the attachTo method is custom written in lua!
	self.myDennis = dennis
	Srv.Game.World:InsertEntity(dennis)
end
