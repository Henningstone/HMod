
print("character loaded")

function Character.OnCreate()
	print(tostring(this), "has been created")

	local dennis = Srv.Game:CreateEntityCustom("Dennis")
	dennis:GetSelf():attachTo(this)  -- the attachTo method is custom written in lua!
	Srv.Game:GetWorld():InsertEntity(dennis)
end

function Character.Tick()
	-- doubletime confirmed!!!11!1
	--this:Tick()
	this:Tick()

	--this:GetCore().Vel = this:GetCore().Vel - vec2(0, 1)
end
