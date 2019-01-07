-- set all the info


config = require "modconfig"
require("botlife")

config.CreateRconInt("dennisness", 0, 9001, 10, "set teh dennisness to over 9000!!")




Srv.Console:Print("dennis", "blub, init running!")

Srv.Console:Register("dennis_is_krass", "r", "much dennis!", function(result)
    Srv.Console:Print("you dennissed!", "with: " .. result:GetString(0)) 
end)

-- command tele_all
Srv.Console:Register("tele_all", "?i", "Tele all players to ID (default yourself)", function(result)
    local TargetID = result:OptInteger(0, result:GetCID())
    local target = Srv.Game:GetPlayerChar(TargetID)

    -- check if target exists
    if target == nil then
        Srv.Console:Print("tele_all", "target player must exist and be ALIVE")
        return
    end

    -- teleport all
    local num = 0
    for CID = 0, 128 do
        local chr = Srv.Game:GetPlayerChar(CID)
        if chr ~= nil then
            print("tele " .. CID .. " from " .. tostring(chr.Pos) .. " to " .. tostring(target.Pos))
            chr.Pos = target.Pos
            chr.Core.Pos = target.Core.Pos
            --chr:Tick()
            num = num + 1
        end
    end

    Srv.Console:Print("tele_all", "teleported " .. num .. " people to " .. Srv.Server:GetClientName(TargetID))
end)

-- command tele
Srv.Console:Register("tele", "i?i", "tele ID to ID (default to yourself)", function(result)
    local VictimID = result:GetInteger(0)
    local victim = Srv.Game:GetPlayerChar(VictimID)
    local TargetID = result:OptInteger(1, result:GetCID())
    local target = Srv.Game:GetPlayerChar(TargetID)

    print("tele " .. VictimID .. " -> " .. TargetID)

    -- check if victim exists
    if victim == nil then
        Srv.Console:Print("tele", "victim player must exist and be ALIVE")
        return
    end

    -- check if target exists
    if target == nil then
        Srv.Console:Print("tele", "target player must exist and be ALIVE")
        return
    end

    -- teleport victim to target
    print("tele " .. VictimID .. " from " .. tostring(victim.Pos) .. " to " .. tostring(target.Pos))
    victim.Pos = target.Pos + vec2(0, -5)
    victim.Core.Pos = target.Core.Pos + vec2(0, -5)

    Srv.Console:Print("tele", "teleported " .. Srv.Server:GetClientName(VictimID) .. " to " .. Srv.Server:GetClientName(TargetID))
end)

-- command create_bot
Srv.Console:Register("create_bot", "", "create a brainless tee", function(result)
    local bot = botlife.BotController:new("BOTTER " .. math.random())
    if bot then
        local BotID = bot:GetCID()
        -- Brainless Tee was successfully created
        Srv.Server:SetClientName(BotID, "Bot " .. BotID)
        Srv.Console:Print("create_bot", "Bot with ID=" .. BotID .. " was created successfully c:")
    else
        Srv.Console:Print("create_bot", "Failed to add bot, is the server full?")
    end
end)

-- command remove_bot
Srv.Console:Register("remove_bot", "i?s", "remove a brainless tee with an optional leave message", function(result)
    local BotID = result:GetInteger(0)
    local bot = botlife.GetByCID(BotID)

    if bot ~= nil and bot:Delete(false, result:OptString(1, "cya nabs")) then
        Srv.Console:Print("remove_bot", "Bot was removed successfully c:")
    else
        Srv.Console:Print("remove_bot", "Invalid bot ID " .. BotID)
    end
end)

Srv.Console:Register("walk_bot", "iii", "make a bot walk towards a point", function(result)
    local BotID = result:GetInteger(0)
    local bot = botlife.GetByCID(BotID)
    if bot ~= nil then
        bot:WalkTowards(vec2(result:GetInteger(1), result:GetInteger(2)))
    else
        Srv.Console:Print("remove_bot", "Invalid bot ID " .. BotID)
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
