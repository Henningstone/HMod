--- LUALIB 'artificial "intelligence" for dummies'
---   This module only exports a class table and can
---   thus safely be imported into the global scope.
----------------------------------------------------------
--- Exports:
---   class BotController
---
--- Methods:
---   BotController:new(name, attach_to) -> BotController|nil
---   BotController:delete(keep_player, leave_msg) -> nil
---   BotController:detach() -> nil
----------------------------------------------------------

module("botlife", package.seeall)

require("timer")

Bots = {}

----------------------------------------------------

--- Bot - Class
---   Represents a simple Bot with serval, helpful functions
--- Methods:
---   Bot:MoveTo( Point, UseHooks, UseJumps )
---     Bot uses a simple algorythm to move towards a given point
---   Bot:Jump()
---     Bot do a simple jump
---   Bot:Shoot( [OPT] Angle, [OPT] WeaponId )
---     Bot shoots with a given (or current) angle and a given (or current active) weapon
---   Bot:Hook( snaps, [OPT] Angle )
---     Bot hooks with for the given snaps with a given (or current) angle
---   Bot:SetAngle( Angle )
---     Sets the angle of the bot
---   Bot:Say( Text, [OPT] ToId )
---     Bot writes a text-message in the chat. If given to a specific playerID (ex. "PlayerWithID: Text")

--- @class Bot
Bot = {
    --- @type CPlayer
    player = nil,

    --- @type SnapController
    snapController = nil,

    --- @type vec2
    wantedPos = nil,

}
Bot.__index = Bot

function Bot:new(Name, Attach_To_Char)
    local BotID = -1
    if Attach_To_Char ~= nil then
        BotID = attach_to:GetCID()
    else
        -- try to create a new bot
        BotID = Srv.Game:CreateBot()
    end

    if BotID == -1 then
        -- error creating bot
        return nil
    end

    -- instantiate the class and initialise our object
    local bot = {}
    setmetatable(bot, Bot)
    bot.player = Srv.Game:GetPlayer(BotID)
    bot.snapController = SnapController:new()

    -- set bot name if wanted
    if name ~= nil then
        Srv.Server:SetClientName(BotID, Name)
    end

    Bots[BotID] = bot
    return bot
end

--- Detaches this BotController from the player it is attached to.
--- @return boolean success
function Bot:Detach()
    if self.player == nil then return false end
    Bots[self.player:GetCID()] = nil
    return true
end

--- Deletes this BotController and the player it is attached to.
--- @param keep_player boolean default:false - if true, the attached player will not be removed from the server.
--- @param leave_msg string optional - reason to use when disconnecting the player
--- @return boolean success
function Bot:Delete(keep_player, leave_msg)
    if self.player == nil then return false end

    keep_player = keep_player or false
    if not keep_player then
        if Srv.Server:ClientIsDummy(self.player:GetCID()) then
            if not Srv.Game:RemoveBot(self.player:GetCID(), leave_msg) then
                print("REMOVE BOT FAILED id=" .. self.player:GetCID())
                return false
            end
            return self:Detach()
        else
            print("KICK PLAYER")
            local kick_msg = "You've been removed from the game"
            if leave_msg ~= nil then
                kick_msg = kick_msg .. " (" .. leave_msg .. ")"
            end
            Srv.Server:Kick(self.player:GetCID(), kick_msg)
            return self:Detach()
        end
    else
        return self:Detach()
    end
end

--- @return number
function Bot:GetCID()
    return self.player:GetCID()
end

--- @return CCharacter
function Bot:GetCharacter()
    return self.player:GetCharacter()
end

--- @return Bot
function GetByCID(ClientID)
    return Bots[ClientID]
end


--- You must call this function at the end of the Player.OnTick callback.
function Bot:Tick()
    local chr = self.player:GetCharacter()
    if chr == nil then return end

    self.snapController:Snap()
end

function Bot:tickAll()
    for _,v in pairs(Bots) do
        if type(v.Tick) == "function" then
            v:Tick()
        end
    end
end

--- Bot - Class
---   Represents a simple Bot with serval, helpful functions
--- Methods:
---   Bot:MoveTo( Point, UseHooks, UseJumps )
---     Bot uses a simple algorythm to move towards a given point
---   Bot:Jump()
---     Bot do a simple jump
---   Bot:Shoot( [OPT] Angle, [OPT] WeaponId )
---     Bot shoots with a given (or current) angle and a given (or current active) weapon
---   Bot:Hook( seconds, [OPT] Angle )
---     Bot hooks with a given (or current) angle
---   Bot:SetAngle( Angle )
---     Sets the angle of the bot
---   Bot:Say( Text, [OPT] ToId )
---     Bot writes a text-message in the chat. If given to a specific playerID (ex. "PlayerWithID: Text")

function Bot:MoveTo( Point, UseHooks, UseJumps )

    local function move(CurSnap, MaxSnaps, ExtraInfo)
        local chr = self.player:GetCharacter()
        if Point.x < chr.Pos.x - 16 then
            chr:GetInput().Direction = -1
        elseif Point.x > chr.Pos.x + 16 then
            chr:GetInput().Direction = 1
        else
            chr:GetInput().Direction = 0
        end

        if UseJumps then
            local dir = chr:GetInput().Direction
            if dir ~= 0 and Srv.Game.Collision:IsTileSolid(chr.Pos.x+dir*32, chr.Pos.y) then
                self:Jump()
            elseif not Srv.Game.Collision:IsTileSolid(chr.Pos.x+dir*12, chr.Pos.y + 12) and chr.Core.Vel.y >= 0 then
                if Srv.Game.Collision:IntersectLine(vec2(chr.Pos.x+dir*12, chr.Pos.y), vec2(chr.Pos.x+dir*12, chr.Pos.y + 32 * 5), nil, nil) == 0 then
                    self:Jump()
                end
            elseif math.abs(Point.x - chr.Pos.x) < 15 and Point.y < chr.Pos.y and chr.Core.Vel.y >= 0 then
                self:Jump()
            end
        end

        if UseHooks then
            -- Hook!
        end
    end

    self.snapController:Append(-1,move,nil,nil,{})
end

function Bot:Jump()
    local chr = self.player:GetCharacter()
    local self = self
    self.snapController:Append(2,
            function(Snap)
                if Snap == 1 then
                    chr:GetInput().Jump = 1
                else
                    chr:GetInput().Jump = 0
                end
            end)
end

function Bot:Shoot(angle)

    self:SetAngle(angle)

    local chr = self.player:GetCharacter()
    self.snapController:Append(1, function(Snap)
            chr:GetInput().Fire = (chr:GetInput().Fire + 1) % 64
        end)
end

function Bot:Hook(snaps, angle)
    local time = tostring(snaps)

    if type(time) ~= "number" then
        error("Hook-Snap count must be a number value!")
    end

    local chr = self.player:GetCharacter()

    self:SetAngle(angle)

    local function s(Snap)
        chr:GetInput().Hook = 1
    end

    local function e()
        chr:GetInput().Hook = 0
    end

    self.snapController:Append(time, s, nil, e)
end

function Bot:SetAngle(angle)

    local pos = vec2(0,0)
    if type(angle) == "userdata" and angle.x and angle.y then
        pos = angle
    elseif type(angle) == "number" then
        local a = angle % 360
        pos = vec2(math.cos(a) * 50, math.sin(a) * 50)
    end
    local chr = self.player:GetCharacter()
    -- U don't even need to use the SnapController! Just overwrite the input like this if its just a 1 time snap :)
    chr:GetInput().TargetX = pos.x
    chr:GetInput().TargetY = pos.y
end

function Bot:Say(Text, ToId)

    ToId = tonumber(ToId)
    if type(ToId == "number") then
        Text = Srv.Server:GetClientName(ToId)..": "..Text
    end
    Srv.Game:SendChat(self.player:GetCID(),tostring(Text))
end

----------------------------------------------------

--- Bot - SnapController
---   Represents a effective Snapping-Controll Class for advanced bot automations
--- Methods:
---
---   SnapController:Append( MaxSnaps, CallbackFunction, [OPT] FilterFunction, [OPT] OnFinishedAllSnaps, [OPT] {...} --[[CallbackExtraParameter]] )
---     Appends a current (master-)Snapping function to the list
---     If MaxSnaps = -1 then it will repeat endlessly!
---     Calls a CallbackFunction function with followed parameter:
---         function CallbackFunction(CurrentSnap, MaxSnaps, [If Given] CallbackExtraParameter)
---         if it returns true, no depencies will getting launched
---     If given, a FilterFunction function gets called to return a boolean which leads into a temporary pause of this Snap (Snaps wont count up for this one, it also dont get launched)
---         function FilterFunction(CurrentSnap, MaxSnaps, [If Given] CallbackExtraParameter)
---     If given, a OnFinishedAllSnaps function gets called when all Snaps are completed (Snaps are bigger or equal to the MaxSnap count)
---         function OnFinishedAllSnaps(MaxSnaps, [If Given] CallbackExtraParameter)
---     * Sets the active ID to this snap
---     returns the ID of the given Snap
---
---   SnapController:Add( CallbackFunction, [OPT] FilterFunction, [OPT] OnFinishedAllSnaps, [OPT] {...} --[[CallbackExtraParameter]]  )
---     Adds a current Snapping function to last entry of the snap list (as a depency, which dont get called if FilterFunction returns false)
---     Calls a CallbackFunction function with followed parameter:
---         function CallbackFunction(CurrentSnap, MaxSnap, [If Given] CallbackExtraParameter)
---         if it returns true, no depencies will getting launched
---     If given, a FilterFunction function gets called to return a boolean which leads into a temporary pause of this Snap (Snaps wont count up for this one, it also dont get launched)
---     If given, a OnFinishedAllSnaps function gets called when all Snaps are completed (Snaps are bigger or equal to the MaxSnap count)
---     * Sets the active ID to this snap
---     returns the ID of the given Snap
---
---   SnapController:Remove( [OPT] ID )
---     Removes the given ID (or last) Snap from the controller.
---     * Sets the active ID to the last valid snap
---     returns true or false whether it has removed it successfully or not
---
---   SnapController:SetActiveID( ID )
---     Sets the current active ID of the snap
---     returns true or false whether it has setted the id successfully or not
---
---   SnapController:GetActiveID()
---     Sets the current active ID of the snap
---     returns the current active ID (ID = 0 if there is no active ID)
---
---   SnapController:GetNextValidID()
---     returns the next valid ID
---
---   SnapController:GetLastValidID()
---     returns the last valid ID
---
---   SnapController:Snap()
---     launches all the Snaps at once!
---
---   SnapController:new()
---     create a new SnapController


--- @class SnapController

SnapController = {
    --- @type table
    --- {
    ---     id = @type number,
    ---     MaxSnaps = @type number,
    ---     CurSnap = @type number,
    ---     CallbackFunction = @type function,
    ---     FilterFunction = @type function,
    ---     OnFinishedAllSnaps = @type function,
    ---     CallbackExtraParameter = @type table,
    ---     Depencies = @type table, -- list of all ID's of depending snaps
    --- }
    snaps = {},

    --- @type number
    activeID = 0,
}
SnapController.__index = SnapController

function SnapController:new()

    local t = {}
    setmetatable(t, SnapController)

    t.activeID = 0
    t.snaps = {}

    return t
end

function SnapController:GetNextValidID()
    local i = 1
    for k,_ in pairs(self.snaps) do
        if not tonumber(k) then
            error("Something shitty happened, got a non number ID of a snap! (id = '"..tostring(k).."')")
        end
        local nk = tonumber(k)
        if nk + 1 > i then
            i = nk + 1
        end
    end
    return i
end

function SnapController:GetLastValidID()
    local i = 0
    for k,_ in pairs(self.snaps) do
        if not tonumber(k) then
            error("Something shitty happened, got a non number ID of a snap! (id = '"..tostring(k).."')")
        end
        local nk = tonumber(k)
        if nk > i then
            i = nk
        end
    end
    return i
end

function SnapController:SetActiveID(ID)
    ID = tonumber(ID)
    if not ID then
        return false
    end
    if ID == 0 then
        self.activeID = ID
        return true
    end
    if self.snaps[ID] then
        self.activeID = ID
        return true
    end
    return false
end

function SnapController:GetActiveID()
   return self.activeID
end

function SnapController:Remove( ID )
    ID = tonumber(ID) or self.activeID
    if ID > 0 then
        if self.snaps(ID) then
            self.snaps[ID] = nil
            self:SetActiveID(self:GetLastValidID())
        end
        return false
    end
    return false
end

function SnapController:Append(MaxSnaps, CallbackFunction, oFilterFunction, oOnFinishedAllSnaps, oCallbackExtraParameter)
    local t = {
        id = self:GetNextValidID(),
        MaxSnaps = MaxSnaps,
        CurSnap = 0,
        CallbackFunction = CallbackFunction,
        FilterFunction = oFilterFunction,
        OnFinishedAllSnaps = oOnFinishedAllSnaps,
        CallbackExtraParameter = oCallbackExtraParameter,
        Depencies = {}
    }

    self.snaps[t.id] = t
    if self:SetActiveID(t.id) then
        return t.id
    end
    return 0
end

function SnapController:Add( CallbackFunction, oFilterFunction, oOnFinishedAllSnaps, oCallbackExtraParameter)
    local t = {
        id = self:GetNextValidID(),
        MaxSnaps = -1,
        CurSnap = 0,
        CallbackFunction = CallbackFunction,
        FilterFunction = oFilterFunction,
        OnFinishedAllSnaps = oOnFinishedAllSnaps,
        CallbackExtraParameter = oCallbackExtraParameter,
        Depencies = {}
    }

    local cId = self.GetActiveID()
    table.insert(self.snaps[cId].Depencies, self.snaps[t.id])

    if self:SetActiveID(t.id) then
        return t.id
    end
    return 0
end

function SnapController:SnapCode(tbl, CurSnap, MaxSnaps)
    if tbl then
        local v = tbl

        local curSnap = CurSnap or v.CurSnap
        local maxSnap = MaxSnaps or v.MaxSnaps

        if type(v.FilterFunction) == "function" then
            if v.FilterFunction(curSnap, maxSnap, v.CallbackExtraParameter) == true then -- Only pause on a explicit TRUE
                return
            else
                v.CurSnap = v.CurSnap + 1
                -- Launch the code!
                if v.CallbackFunction(v.CurSnap, v.MaxSnaps, v.CallbackExtraParameter) == true then
                    -- Dont launch depencies
                else
                    for k,vv in pairs(v.Depencies) do
                        self:SnapCode(vv,curSnap, maxSnap)
                    end
                end
            end
        else
            v.CurSnap = v.CurSnap + 1
            -- Launch the code!
            if v.CallbackFunction(v.CurSnap, v.MaxSnaps, v.CallbackExtraParameter) == true then
                -- Dont launch depencies
            else
                for k,vv in pairs(v.Depencies) do
                    self:SnapCode(vv,curSnap, maxSnap)
                end
            end
        end
        if v.CurSnap >= v.MaxSnaps and v.MaxSnaps ~= -1 then
            if type(v.OnFinishedAllSnaps) == "function" then
                v.OnFinishedAllSnaps(v.CallbackExtraParameter)
            end
            self.snaps[v.id] = nil
            return
        end
    end
end

function SnapController:Snap()
    for k,v in pairs(self.snaps) do
        self:SnapCode(v)
    end
end


--- @module botlife
local m = {
    BotController = Bot,
    SnapController = SnapController,
    GetByCID = GetByCID
}
return m
