
print("character loaded")


function Character.OnInsert()
	print("Character", tostring(this), "has been created")
	this:GetPlayer():GetSelf():enableDennis()

	if this:GetPlayer():GetSelf().prev_tuning ~= nil then
		print("applying previous tuning!!")
		this.Tuning = this:GetPlayer():GetSelf().prev_tuning:Copy()
	end
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
	this:GetPlayer():GetSelf().prev_tuning = this.Tuning:Copy()
	print(tostring(this:GetPlayer():GetSelf().prev_tuning))
end

function Character.OnWeaponFire(Weapon)
	this:GetPlayer():GetSelf():triggerDennis(Weapon)
end
