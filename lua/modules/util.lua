--- LUALIB 'generally useful utilities'
----------------------------------------------------------
--- Exports:
---   int(var)
---   argcheck(arg, narg, expected)
----------------------------------------------------------


module("util", package.seeall)


--- Converts a boolean, string or decimal number to a whole number (performs proper rounding)
--- @param var boolean|string|number What to convert
--- @return number|nil The result or nil if the given value was invalid
function int(var)
    local t = type(var)
    if t == "boolean" then return var and 1 or 0 end
    if t == "string" or t == "number" then return math.floor(tonumber(var)+0.5) end
    return nil
end

--- Raises an error if the argument does is not of the expected type
--- @param arg any the argument to check
--- @param narg number the index of the argument to check
--- @param expected string|table the name of the expected type
function argcheck(arg, narg, expected)
    if type(expected) ~= "table" then
        expected = { expected }
    end

    local argtype = type(arg)

    local found = false
    for i, v in ipairs(expected) do
        if v == argtype then
            found = true
            break
        end
    end

    if not found then
        error("bad argument #" .. narg .. " (" .. expected .. " expected, got " .. argtype .. ")", 3)
    end
end


--- @module util
local m = {
    int = int,
    argcheck = argcheck
}
return m
