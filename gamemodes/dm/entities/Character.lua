
print("character loaded")

function Character.OnCreate()
	print(tostring(this), "has been created")

	local dennis = Srv.Game:CreateEntityCustom("Dennis")
	dennis:GetSelf():attachTo(this)
	Srv.Game:GetWorld():InsertEntity(dennis)
end

function Character.Tick()
	-- doubletime confirmed!!!11!1
	this:Tick()
	this:Tick()
end
