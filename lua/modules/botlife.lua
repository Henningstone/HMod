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

local _bots = {}


--- @class BotController
BotController = {
    --- @type CPlayer
    player = nil,
    --- @type vec2
    goal_point = nil,
    --- @type boolean
    auto_jump = nil,
}
BotController.__index = BotController


--- Constructor.
--- @param name string Optional name to set for this dummy on the server.
--- @param attach_to CPlayer Optional player to attach to. No new bot will be created, if given.
--- @return BotController|nil returns a new instance of BotController or nil if no new bot could be created on the server
function BotController:new(name, attach_to)

    local BotID = -1
    if attach_to ~= nil then
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
    setmetatable(bot, BotController)
    bot.player = Srv.Game:GetPlayer(BotID)

    -- set bot name if wanted
    if name ~= nil then
        Srv.Server:SetClientName(BotID, Name)
    end

    _bots[bot:GetCID()] = bot
    return bot
end

--- static function to Tick all bots at once
function BotController:tickAll()
    for _,bot in next,_bots do
        bot:Tick()
    end
end

local function reset(self)
    self.player = nil
    self.goal_point = nil
    self.auto_jump = nil
end

--- Deletes this BotController and the player it is attached to.
--- @param keep_player boolean default:false - if true, the attached player will not be removed from the server.
--- @param leave_msg string optional - reason to use when disconnecting the player
--- @return boolean success
function BotController:Delete(keep_player, leave_msg)
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

--- Detaches this BotController from the player it is attached to.
--- @return boolean success
function BotController:Detach()
    if self.player == nil then return false end
    _bots[self.player:GetCID()] = nil
    reset(self)
    return true
end

--- You must call this function at the end of the Player.OnTick callback.
function BotController:Tick()
    local chr = self.player:GetCharacter()
    if chr == nil then return end

    timer.Tick()

    -- handle walking towards a point
    if self.goal_point ~= nil then
        -- figure out walking direction
        if self.goal_point.x < chr.Pos.x - 12 then
            chr.Core.Input.Direction = -1
            print("WALK LEFT")
        elseif self.goal_point.x > chr.Pos.x + 12 then
            chr.Core.Input.Direction = 1
            print("WALK RIGHT")
        else
            chr.Core.Input.Direction = 0
            print("WALK STOP")
        end
        print(chr.Core.Input.Direction)

        -- do we want to jump?
        if self.auto_jump then
            local wd = chr.Core.Input.Direction

            -- do we need to jump?
            if wd ~= 0 and Srv.Game.Collision:IsTileSolid(chr.Pos.x+wd*12, chr.Pos.y) then
                chr.Core.Input.Jump = 1
                timer.Start(0.5, false, function () chr.Core.Input.Jump = 0 end)
            end
        end
    end
end

--- Makes the bot walk towards the given point
--- @param point vec2
--- @param auto_jump boolean try to jump over obstacles
function BotController:WalkTowards(point, auto_jump)
    self.goal_point = point
    self.auto_jump = auto_jump
end

--- @return number
function BotController:GetCID()
    return self.player:GetCID()
end

--- @return CCharacter
function BotController:GetCharacter()
    return self.player:GetCharacter()
end

--- @return BotController
function GetByCID(ClientID)
    return _bots[ClientID]
end

--- @module botlife
local m = {
    BotController = BotController,
    GetByCID = GetByCID
}
return m
