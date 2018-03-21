
Pickup.count = 0
Pickup._count = 0  -- static

Pickup.dennis = { c=Pickup, a={ b=2 } }

Pickup.name = "<pickup>"


function Pickup.OnCreate()
	self.count = self.count + 1
	Pickup._count = Pickup._count + 1
	Pickup["count"] = Pickup.count + 1
	--Pickup["somebullshit"] = "dennis"
	self.dennis.a.b = self.dennis.a.b + 2
	print(tostring(this), tostring(self), self.__dbgId, "has been created", self.count, Pickup._count, self.dennis.a.b)

	--local t = getmetatable(Pickup)
	--print(tostring(t))

	--for k,v in next,t do print(tostring(k), tostring(v)) end
	--t.__newindex(Pickup, "dennis", 42)
	--if x then IServer.Tick = 42 end
	--x = true


--print("iserver: "..tostring(IServer))
	--IServer.Tick = 42
--	print(tostring(IServer.Tick))
--	IServer.Tick = 312
--[[
print("------------------------------------------ START")
	local t = getmetatable(IServer)
	printTbl(t, "t")
	printTbl(t.__class, "t.__class")
	printTbl(t.__propset, "t.__propset")
	printTbl(t.__propget, "t.__propget")
print("END ------------------------------------------")

print(type(Pickup), type(Srv.Server))
]]
end

function Pickup.OnPickedUp(Chr)
	local CID = Chr:GetPlayer():GetCID()
	local Name = Srv.Server:GetClientName(CID)
	print(string.format("DENNNNN NNNNIIISS    %s (%d) has picked up '%s'", Name, CID, self.name))

	return self.name ~= "<pickup>"
end

function Pickup.setName(self, NewName)
	print("PPPPPPIIIICKUP SETTINGS NAME " .. self.name .. " -> " .. NewName)
	self.name = NewName
end


function printTbl(t, name)
	print("## " .. (name or '(table)'))
	for k,v in next,t do print(tostring(k),tostring(v),type(v)) end
end


     function settable_event (table, key, value)
       local h
       if type(table) == "table" then
         local v = rawget(table, key)
         print("v: " .. tostring(v))
         if v ~= nil then rawset(table, key, value); return end
         print("STILL HERE")
         h = metatable(table).__newindex
         if h == nil then rawset(table, key, value); return end
       else
         h = metatable(table).__newindex
         if h == nil then
           error(···)
         end
       end
       if type(h) == "function" then
         h(table, key,value)           -- call the handler
       else h[key] = value             -- or repeat operation on it
       end
     end
