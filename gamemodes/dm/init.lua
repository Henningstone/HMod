-- set all the info
print("++++++++++LUA ALIVE!++++++++++++++++")

Server.Console:Print(0, "dennis", "blub")

Server.Console:Register("dennis_is_krass", "r", "much dennis!", function(result)
	Server.Console:Print(0, "you dennissed!", "with: " .. result:GetString(0)) 
end)

Server.Console:Register("make_dennis", "", "dew it!", function(result)
	local ninjadude = Server.Game:CreateEntityPickup(2, 3) -- 3 5
	ninjadude:BindClass("Dennis")
	table.insert(Character.ents, ninjadude)
	print(#Character.ents, tostring(ninjadude))
	Server.Console:Print(0, "make_dennis", "made a dennis!")
end)
