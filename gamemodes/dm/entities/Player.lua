

Player.myDennis = nil


function Player.Player()
	print("player ctor")
end



function Player.OnDisconnect(Reason)
	print(Srv.Server:GetClientName(this:GetCID()) .." disconnected with reason " .. Reason)

	if self.myDennis ~= nil then
		Srv.Game.World:DestroyEntity(self.myDennis)
		self.myDennis = nil
	end
end


-- called from rcon cmd callback
function Player.createDennis(self, this)
	if self.myDennis ~= nil then return false end

	print("Player", tostring(this), "gets his dennis")

	local dennis = Srv.Game:CreateEntityCustom("Dennis")
	dennis:GetSelf():attachTo(this)  -- the attachTo method is custom written in lua!
	self.myDennis = dennis
	Srv.Game.World:InsertEntity(dennis)

    -- if our character is alive, show it immediately
    if this:GetCharacter() ~= nil then
        self:enableDennis()
    end

	return true
end

function Player.enableDennis(self)
	if self.myDennis == nil then return end

	if not self.myDennis:GetSelf():spawn() then
		error("dennis didn't want to spawn")
	end
end

function Player.disableDennis(self)
	if self.myDennis == nil then return end

	if not self.myDennis:GetSelf():disappear() then
		error("dennis didn't want to disappear")
	end
end

function Player.triggerDennis(self, Weapon)
	if self.myDennis == nil then return end

	self.myDennis:GetSelf():trigger(Weapon+1)
end
