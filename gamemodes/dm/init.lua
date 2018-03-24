-- set all the info

Srv.Console:Print("dennis", "blub, init running!")

Srv.Console:Register("dennis_is_krass", "r", "much dennis!", function(result)
	Srv.Console:Print("you dennissed!", "with: " .. result:GetString(0)) 
end)

Srv.Console:Register("make_dennis", "", "dew it!", function(result)
	local ninjadude = Srv.Game:CreateEntityPickup(2, 3) -- 3 5
	ninjadude:BindClass("Dennis")
	table.insert(Character.ents, ninjadude)
	print(#Character.ents, tostring(ninjadude))
	Srv.Console:Print("make_dennis", "made a dennis!")
end)

Srv.Console:Register("give_amazingness_dennis", "i?i?i?i", "Give'em teh real shit bro (ID, NUM_RINGS, ENTS_PER_RING, MAX_CAPT_DIST)", function(result)
	local GiveTo = result:GetInteger(0)

	local pl = Srv.Game:GetPlayer(GiveTo)
	if pl == nil then
		Srv.Console:Print("dennisdude", "There is no player with ID " .. GiveTo)
		return
	end

	local NumRings = result:OptInteger(1, 3)
	local EntsPerRing = result:OptInteger(2, 8)
	local MaxCaptDist = result:OptInteger(3, 500)

	local Success = pl:GetSelf():createDennis(pl, NumRings, EntsPerRing, MaxCaptDist)
	if Success == true then
		Srv.Console:Print("dennisdude", "Given awesomeness to " .. Srv.Server:GetClientName(GiveTo))
	else
		Srv.Console:Print("dennisdude", Srv.Server:GetClientName(GiveTo) .. " has already got awesomeness!")
	end
end)

Srv.Console:Register("remove_dennis", "i", "he not worth dem awesomenezz! REMOVE!", function (result)
	local CID = result:GetInteger(0)

	local pl = Srv.Game:GetPlayer(CID)
	if pl == nil then
		Srv.Console:Print("dennisdude", "There is no player with ID " .. CID)
		return
	end

	local Success = pl:GetSelf():destroyDennis(pl)
	if Success == true then
		Srv.Console:Print("dennisdude", "Removed awesomeness of " .. Srv.Server:GetClientName(CID))
	else
		Srv.Console:Print("dennisdude", Srv.Server:GetClientName(CID) .. " is not so very awesome already!")
	end

end)


print("++++++++++LUA INIT DONE++++++++++++++++")
