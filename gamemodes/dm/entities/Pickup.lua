
Pickup.count = 0

Pickup.dennis = { c=Pickup, a={ b=2 } }

function Pickup.OnCreate()
	self.count = self.count + 1
	self.dennis.a.b = self.dennis.a.b + 2
	print(tostring(this), tostring(self), self.__dbgId, "has been created", self.count, self.dennis.a.b)
end
