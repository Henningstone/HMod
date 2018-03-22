-- set all the info

Srv.Console:Print(0, "dennis", "blub")

Srv.Console:Register("dennis_is_krass", "r", "much dennis!", function(result)
	Srv.Console:Print(0, "you dennissed!", "with: " .. result:GetString(0)) 
end)

Srv.Console:Register("make_dennis", "", "dew it!", function(result)
	local ninjadude = Srv.Game:CreateEntityPickup(2, 3) -- 3 5
	ninjadude:BindClass("Dennis")
	table.insert(Character.ents, ninjadude)
	print(#Character.ents, tostring(ninjadude))
	Srv.Console:Print(0, "make_dennis", "made a dennis!")
end)

Srv.Console:Register("give_amazingness_dennis", "i", "Give'em teh real shit bro", function(result)
	local GiveTo = result:GetInteger(0)
	local chr = Srv.Game:GetPlayerChar(GiveTo)
	if chr == nil then
		Srv.Console:Print(0, "dennisdude", "There is no player with ID " .. GiveTo)
		return
	end
	Srv.Console:Print(0, "dennisdude", "Giving awesomeness to " .. Srv.Server:GetClientName(GiveTo))

	chr:GetSelf():spawnMyDennis(chr)

end)


print("++++++++++LUA INIT DONE++++++++++++++++")
