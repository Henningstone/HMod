-- set all the info


config = require "modconfig"

config.CreateRconInt("dennisness", 0, 9001, 10, "set teh dennisness to over 9000!!")




Srv.Console:Print("dennis", "blub, init running!")

Srv.Console:Register("dennis_is_krass", "r", "much dennis!", function(result)
    Srv.Console:Print("you dennissed!", "with: " .. result:GetString(0)) 
end)

Srv.Console:Register("tele_all_here", "", "some legit shit SeemsGood", function(result)
    local me = Srv.Game:GetPlayerChar(result:GetCID())
    if me == nil then
        Srv.Console:Print("tele_all_here", "you must be alive") 
        return
    end

    local num = 0
    for CID = 0, 128 do
        local chr = Srv.Game:GetPlayerChar(CID)
        if chr ~= nil then
            print("tele " .. CID .. " from " .. tostring(chr.Pos) .. " to " .. tostring(me.Pos))
            chr.Pos = me.Pos
            chr.Core.Pos = me.Core.Pos
            --chr:Tick()
            num = num + 1
        end
    end

    Srv.Console:Print("tele_all_here", "teleported " .. num .. " people to you!") 

end)

Srv.Console:Register("create_bot", "i", "create a brainless tee", function(result)
    local BotID = result:GetInteger(0)

    if Srv.Game:CreateBot(BotID) then
        -- Brainless Tee was successfully created
        Srv.Console:Print("create_bot", "Bot was created successfully c:") 
    else
        Srv.Console:Print("create_bot", "Better you dont ask what happened :c") 
    end
end)

Srv.Console:Register("remove_bot", "i", "remove a brainless tee", function(result)
    local BotID = result:GetInteger(0)

    if Srv.Game:RemoveBot(BotID) then
        -- Brainless Tee was successfully created
        Srv.Console:Print("remove_bot", "Bot was removed successfully c:") 
    else
        Srv.Console:Print("remove_bot", "Better you dont ask what happened :c") 
    end
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
