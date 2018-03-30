--- LUALIB 'configuration management'
----------------------------------------------------------
--- Exports:
---   function Set(key, value)
---   function Get(key)
---   function Load()
---   function Save()
---   CreateRconString(name, default, description)
---   CreateRconInt(name, min, max, default, description)
----------------------------------------------------------

module("modconfig", package.seeall)

local stringutils = require("stringutils")
local util = require("util")

local _scriptConfig = {}
local _scriptConfigFile = "autoconfig.json"


--- Sets the value of an internal config variable (it will not be accessible through the rcon)
--- If the config variable does not yet exist, it will be created.
--- @param key string|number Identifier of this config variable.
--- @param value nil|number|boolean|string|table The new value of this config variable
--- @return void
function Set(key, value)
    if key == nil then return end
    _scriptConfig[key] = value
end

--- Receive the value of a config variable. If the variable does not exist, returns nil.
--- @param key string|number Identifier of the variable to get
--- @return any The previously set value of the variable or nil if it doesn't exist.
function Get(key)
    if key == nil then return nil end
    return _scriptConfig[key]
end

--- Load the configuration from the config file.
--- @return boolean,string A bool indicating success and the used path of the config file
function Load()
    local ConfFile = _scriptConfigFile
    if Config.debug > 0 then print("[config] Loading settings from '" .. ConfFile .. "'") end

    local f,path = io.open(_scriptConfigFile)

    if f == nil then
        -- try alternative
        f,path = io.open(g_ScriptConfigFileOld)
    end

    if f == nil then
        if Config.debug > 0 then
            print("[config] Failed. No settings file found.")
        end
        return false, path
    end

    local json_string = f:read("*a")
    local success, result = pcall(json.Parse, json_string) -- lua style try-catch - result is either our json_value or an error message
    f:close()

    -- json parsing error
    if not success then
        throw("[config] Failed to load config! " .. result)
        return false, path
    end

    success, result = pcall(result.ToObject, result)
    if not success then
        throw("[config] Failed to load config! " .. result)
        return false, path
    end

    _scriptConfig = result

    if Config.debug > 0 then
        print("[config] Done! Loaded settings from '" .. path .. "'")
    end
    return true, path
end

--- Write the config to the disk
function Save() -- call this in OnScriptUnload()
    local file = io.open(_scriptConfigFile, "w+")
    if file == nil then error("Failed to save config to " .. _scriptConfigFile, 2) end

    local json_value = json.Convert(_scriptConfig)
    local json_string = json.Serialize(json_value)
    json_value:Destroy()

    file:write(json_string)
    file:flush()
    file:close()
    if Config.debug > 0 then print("[config] Saved settings to '" .. _scriptConfigFile .. "'") end
end

--- Creates a config variable that can only contain a string and makes it accessible from the remote console.
--- @param name string the name of the variable. In the rcon, it will start with sv_mod_
--- @param default string the default value for this variable at server start
--- @param description string description of this config var to be displayed in the remote console
function CreateRconString(name, default, description)
    util.argcheck(name, 1, 'string')
    util.argcheck(default, 2, 'string')
    util.argcheck(description, 3, {'string','nil'})
    description = description or ""

    -- create an internal config var to hold the value
    Set(name, default)

    -- make sure rcon config vars have consistent naming
    local prefix = "sv_" .. Config.sv_gametype:lower() .. "_"
    local varname = name
    if not stringutils.StartsWith(varname, prefix) then
        varname = prefix .. varname
    end

    -- register it to the console
    Srv.Console:Register(varname, "?r", description, function(result)
        if result:NumArguments() == 0 then
            Srv.Console:Print("Console", "Value: " .. Get(name))
        else
            Set(name, result:GetString(0))
        end
    end)
end

--- Creates a config variable that can only contain an integer and makes it accessible from the remote console.
--- @param name string the name of the variable. In the rcon, it will start with sv_mod_
--- @param min int the smalles value this variable can have
--- @param max int the largest value this variable can have
--- @param default int the default value for this variable at server start
--- @param description string description of this config var to be displayed in the remote console
function CreateRconInt(name, min, max, default, description)
    util.argcheck(name, 1, 'string')
    util.argcheck(min, 2, 'number')
    util.argcheck(max, 3, 'number')
    util.argcheck(default, 4, 'number')
    util.argcheck(description, 5, {'string','nil'})
    description = description or ""

    -- create an internal config var to hold the value
    Set(name, default)

    -- make sure rcon config vars have consistent naming
    local prefix = "sv_" .. Config.sv_gametype:lower() .. "_"
    local varname = name
    if not stringutils.StartsWith(varname, prefix) then
        varname = prefix .. varname
    end

    -- register it to the console
    Srv.Console:Register(varname, "?i", description, function(result)
        if result:NumArguments() == 0 then
            Srv.Console:Print("Console", "Value: " .. Get(name))
        else
            -- get the new value and clamp it
            local new_val = result:GetInteger(0)
            if new_val < min then new_val = min end
            if new_val > max then new_val = max end
            Set(name, new_val)
        end
    end)
end


--- @module config
local m = {
    Set = Set,
    Get = Get,
    Load = Load,
    Save = Save,
    CreateRconString = CreateRconString,
    CreateRconInt = CreateRconInt
}
return m
