
Pickup._count = 3

Pickup.CanNotBePickedUp = false

function Pickup.OnCreate()

	Pickup._count = Pickup._count + 1

	print(tostring(this), tostring(self), self.__dbgId, "has been created", Pickup._count)

end

function Pickup.OnPickedUp(Chr)
	return self.CanNotBePickedUp
end
