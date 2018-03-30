--- LUALIB 'common functions for processing strings'
----------------------------------------------------------
--- Exports:
---   StartsWith(str, find)
---   Split(str, split)
---   RTrim(str)
---   UnpackArgs(str, delim)
---   FromTable(tbl, delim)
----------------------------------------------------------

module("stringutils", package.seeall)


--- Checks whether a string starts with the given substring
--- @param str string String to search in
--- @param find string String to check for
--- @return boolean
function StartsWith(str, find)
    if str:sub(1, find:len()) == find then return true else return false end
end

--- Splits the given string once at the given substring.
--- The substring itself will not be included in either of the resulting parts.
--- @param str string The string to split
--- @param split string The substring to split at
--- @return string,string
function Split(str, split)
    if type(str) ~= "string" or type(split) ~= "string" then
        error("string expected, got " .. type(str), 2)
    end

    local a, b = string.find(str, split, 1, true)
    if (a == nil or b == nil) then
        return "",""
    end
    return string.sub(str, 0, a-1), string.sub(str, b+1, -1)
end

--- Removes spaces at the end of a string
--- @param str string String to trim
--- @return string the trimmed string
function RTrim(str)
	local n = #str
	while n > 0 and str:find("^%s", n) do n = n - 1 end
	return str:sub(1, n)
end

--- Splits a string by spaces into a table.
--- @param str string Space-separated list of arguments
--- @param delim string optional; define what to split by instead of space. Default is a space character.
--- @return table
function UnpackArgs(str, delim)
    delim = delim or " "
    local tbl = {}
    str = RTrim(str)
    local index = str:find(delim)
    while true do
        if index == nil then
            table.insert(tbl, str)
            break
        end
        table.insert(tbl, str:sub(0, index-1))
        str = str:sub(index+1, -1)
        index = str:find(delim)
    end
    return tbl
end

--- Assembles a table into one coherent string
--- @param tbl table ipairs-iterable table
--- @param delim string which seperator to use between the elements
--- @return string all elements of a table stringified and stuck together with the given seperator inbetween
function StringFromTable(tbl, delim)
    delim = delim or " "
    local str = ""
    for i,v in ipairs(tbl) do
        str = str .. tostring(v) .. delim
    end
    str = str:sub(1, -(1 + #delim))
    return str
end


--- @module stringutils
local m = {
	StartsWith = StartsWith,
	Split = Split,
	RTrim = RTrim,
	UnpackArgs = UnpackArgs,
	StringFromTable = StringFromTable
}
return m
