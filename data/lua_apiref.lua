

-- -- -- -- -- -- -- [ GROUP: Custom Types ] -- -- -- -- -- -- --

--- @class vec2
--- A two-dimensional vector, commonly used for positions or directions.
--- Uniform attributes:
--- / x,u
--- / y,v
vec2 = {
    --- @type number
    x = nil,
    --- @type number
    y = nil,
    --- @type number
    u = nil,
    --- @type number
    v = nil,
}
--- Constructor
--- @param x number
--- @param y number
--- @return vec2
function vec2(x, y) end



--- @class vec3
--- A three-dimensional vector, commonly used for RGB/HSL/HSV colours.
--- Uniform attributes:
--- / x,r,h
--- / y,g,s
--- / z,b,l,v
vec3 = {
    --- @type number
    x = nil,
    --- @type number
    y = nil,
    --- @type number
    z = nil,
    --- @type number
    r = nil,
    --- @type number
    g = nil,
    --- @type number
    b = nil,
    --- @type number
    h = nil,
    --- @type number
    s = nil,
    --- @type number
    l = nil,
    --- @type number
    v = nil,
}
--- Constructor
--- @param r number
--- @param g number
--- @param b number
--- @return vec3
function vec3(r, g, b) end



--- @class vec4
--- A four-dimensional vector, commonly used for RGBA colours.
--- Uniform attributes:
--- / r,x
--- / g,y
--- / b,z,u
--- / a,w,v
vec4 = {
    --- @type number
    r = nil,
    --- @type number
    g = nil,
    --- @type number
    b = nil,
    --- @type number
    a = nil,
    --- @type number
    x = nil,
    --- @type number
    y = nil,
    --- @type number
    z = nil,
    --- @type number
    w = nil,
    --- @type number
    u = nil,
    --- @type number
    v = nil,
}
--- Constructor
--- @param r number
--- @param g number
--- @param b number
--- @param a number
--- @return vec4
function vec4(r, g, b, a) end



--- @class CNetObj_PlayerInput
CNetObj_PlayerInput = {
    --- @type number
    Direction = nil,
    --- @type number
    TargetX = nil,
    --- @type number
    TargetY = nil,
    --- @type number
    Jump = nil,
    --- @type number
    Fire = nil,
    --- @type number
    Hook = nil,
    --- @type number
    PlayerFlags = nil,
    --- @type number
    WantedWeapon = nil,
    --- @type number
    NextWeapon = nil,
    --- @type number
    PrevWeapon = nil,
}


-- -- -- -- -- -- -- [ GROUP: CPlayer ] -- -- -- -- -- -- --

--- @class CPlayer
CPlayer = {
    --- @type vec2
    ViewPos = nil,
    --- @type number
    PlayerFlags = nil,
    --- @type number
    SpectatorID = nil,
    --- @type boolean
    IsReady = nil,
}

--- @return nil
function CPlayer:TryRespawn() end

--- @return nil
function CPlayer:Respawn() end

--- @return number
function CPlayer:GetTeam() end

--- @param Team number Team to join
--- @param DoChatMessage boolean Whether to announce the team change in chat
function CPlayer:SetTeam(Team, DoChatMessage) end

--- @return number
function CPlayer:GetCID() end

function CPlayer:Tick() end
function CPlayer:PostTick() end
function CPlayer:Snap() end

--- Adds a client info snap for this player to the current snap batch.
--- **IMPORTANT**: only use this in the **Player.Snap** event callback!
--- @param Name string
--- @param ClanName string
--- @param Country number
--- @param SkinName string
--- @param UseCustomColor boolean
--- @param ColorBody number
--- @param ColorFeet number
--- @return boolean true on success, otherwise false
function CPlayer:AddClientInfoSnap(Name, ClanName, Country, SkinName, UseCustomColor, ColorBody, ColorFeet) end

--- Adds a player info snap for this player to the current snap batch.
--- **IMPORTANT**: only use this in the **Player.Snap** event callback!
--- @param Score number
--- @param Team number
--- @param Latency number
--- @param Local boolean Whether the SnappingClient is _this_ player, should **ALWAYS** and **ONLY** be _true_ if SnappingClient == this:GetCID(), otherwise weird things might happen for the client!
--- @return boolean true on success, otherwise false
function CPlayer:AddPlayerInfoSnap(Score, Team, Latency, Local) end

--- Adds a spectator info snap for this player to the current snap batch.
--- **IMPORTANT**: only use this in the **Player.Snap** event callback!
--- @param SpectatorID number whom the player is spectating now.
--- @param X number
--- @param Y number
function CPlayer:AddSpectatorInfoSnap(SpectatorID, X, Y) end

--- Internally used to process fresh input messages sent by the client.
--- Note: Despite the 'On' prefix, this is not an event function.
--- @param NewInput CNetObj_PlayerInput
function CPlayer:OnDirectInput(NewInput) end

--- Internally used to predict input based on previously sent input messages.
--- Note: Despite the 'On' prefix, this is not an event function.
--- @param NewInput CNetObj_PlayerInput
function CPlayer:OnPredictedInput(NewInput) end

--- TODO continue here with OnDisconnect (which is a event function!)
