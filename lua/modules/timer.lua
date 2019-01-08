--- LUALIB 'automatic trigger timer'
----------------------------------------------------------
--- Exports:
---   function Set(key, value)
---   function Get(key)
---   function Load()
---   function Save()
---   CreateRconString(name, default, description)
---   CreateRconInt(name, min, max, default, description)
----------------------------------------------------------

module("timer", package.seeall)

local _timers = {}


--- Start a new timer.
--- @param seconds number In how many seconds to call the function
--- @param is_interval boolean If set to true, the timer will run in intervals of 'seconds'
--- @param callback function Function to call in X seconds
--- @param self table|nil optional - Will be passed as the 'self' of to the callback if not nil
--- @vararg any optional - Arguments to pass to the callback function
--- @return number An identifier for this timer to use in Stop
function Start(seconds, is_interval, callback, self, ...)
    if seconds < 0 then error("'seconds' may not be less than zero, got " + seconds, 2) end
    if type(callback) ~= 'function' then error("'callback' must be a function, got " + type(callback), 2) end
    if is_interval and seconds == 0 then error("'seconds' can not be zero for interval timers", 2) end

    local now = Srv.Server:Tick()
    local end_tick = now + seconds*Srv.Server:TickSpeeed()

    local t = {
        stop_tick = end_tick,
        callback = callback,
        self = self,
        args = {...},
        interval = is_interval and seconds or false
    }

    table.insert(_timers, t)
    return #_timers
end


--- This needs to be called in a Tick event. It will update timers
--- and fire callback functions for timers that ran out.
function Tick()
    local now = Srv.Server.Tick
    local finished = {}

    for i,t in next,_timers do
        if now > t.end_tick then
            -- timer ran out, fire it off
            if t.self ~= nil then
                t.callback(t.self, t.args)
            else
                t.callback(t.args)
            end

            -- update interval timers or queue for deletion
            if t.interval then
                local end_tick = now + t.interval*Srv.Server:TickSpeeed()
                t.end_tick = end_tick
            else
                table.insert(finished, i)
            end
        end
    end

    -- delete finished timers
    for _,i in next,finished do
        table.remove(_timers, i)
    end
end

--- Stop a timer.
---
function Stop(identifier)
    table.remove(_timers, identifier)
end



--- @module timer
local m = {
    Start = Start,
    Tick = Tick,
    Stop = Stop,
}
return m
