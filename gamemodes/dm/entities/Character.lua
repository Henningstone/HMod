
print("character loaded")


function Character.OnCreate()
	print("Character", tostring(this), "has been created")
	this:GetPlayer():GetSelf():enableDennis()
end

--[[
function Character.Destroy()
	print("Character", tostring(this), "has been destroyed")
	this:GetPlayer():GetSelf():disableDennis()
end
]]

function Character.OnDeath(Killer, Weapon)
	print("Character", tostring(this), "was kileld by " .. Killer .. " with " .. Weapon)
	this:GetPlayer():GetSelf():disableDennis()
end

function Character.OnWeaponFire(Weapon)
	this:GetPlayer():GetSelf():triggerDennis(Weapon)
end
