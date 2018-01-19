
print("character loaded")

Character.ents = {}

function Character.Tick()
	--print("char tick", tostring(this), tostring(self))
	this:Tick()
	this:Tick()

	for i,_ in ipairs(Character.ents) do
		self.ents[i].m_Pos = this.m_Pos + vec2(0, -50)

		--print(i, tostring(Character.ents), tostring(self.ents[i].m_Pos))
	end
end

function Character.Snap(SnappingClient)
	this:Snap(SnappingClient)
end
