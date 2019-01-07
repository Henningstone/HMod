
botlife = require 'botlife'


Player.myDennis = nil
Player.isCaptured = false

function Player.Player()
	print("player ctor")
end


function Player.Tick()
	this:Tick()
	botlife.BotController:tickAll()
end


function Player.OnDisconnect(Reason)
    Reason = Reason or "(no reason given)"
	print(Srv.Server:GetClientName(this:GetCID()) .." disconnected with reason " .. Reason)

	self:destroyDennis()
end


-- called from rcon cmd callback
function Player.createDennis(self, this, NumRings, EntsPerRing, MaxCaptDist)
	if self.myDennis ~= nil then return false end

	print("Player", tostring(this), "gets his dennis")

	local dennis = Srv.Game:CreateEntityCustom("Dennis")
	local dennisSelf = dennis:GetSelf()

	dennisSelf.NUM_RINGS = NumRings or dennisSelf.NUM_RINGS
	dennisSelf.NUM_ITEMS = EntsPerRing or dennisSelf.NUM_ITEMS
	dennisSelf.MAX_CAPTURE_DIST = MaxCaptDist or dennisSelf.MAX_CAPTURE_DIST

	dennisSelf:attachTo(this)  -- the attachTo method is custom written in lua!
	self.myDennis = dennis

	Srv.Game.World:InsertEntity(dennis)

    -- if our character is alive, show it immediately
    if this:GetCharacter() ~= nil then
        self:enableDennis()
    end

	return true
end

function Player.destroyDennis(self)
	if self.myDennis ~= nil then
		Srv.Game.World:DestroyEntity(self.myDennis)
		self.myDennis = nil
		return true
	else
		return false
	end
end

function Player.enableDennis(self, triggeredByCapture)
	if self.myDennis == nil then return end
	triggeredByCapture = triggeredByCapture or false

	if triggeredByCapture then
		self.isCaptured = false
	elseif self.isCaptured then
		-- we are captured, so can't re-enable on respawn
		return
	end

	if not self.myDennis:GetSelf():spawn() then
		error("dennis didn't want to spawn")
	end
end

function Player.disableDennis(self, triggeredByCapture)
	if self.myDennis == nil then return end
	triggeredByCapture = triggeredByCapture or false

	if triggeredByCapture then
		self.isCaptured = true
	elseif self.isCaptured then
		-- it's not there anyways, so nothing to disable
		return
	end

	if not self.myDennis:GetSelf():disappear() then
		error("dennis didn't want to disappear")
	end
end

function Player.triggerDennis(self, Weapon)
	if self.myDennis == nil then return end

	self.myDennis:GetSelf():trigger(Weapon+1)
end
