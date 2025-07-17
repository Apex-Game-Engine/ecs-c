local CMoney = {}
CMoney.__index = CMoney
CMoney.balance = 0

function CMoney:Add(amt)
	if self.balance + amt < 0 then
		throw_error("Insufficient funds")
		return
	end
	self.balance = self.balance + amt
end

function CMoney:Balance(amt)

end

ecs.RegisterComponent(CMoney, "Money")

for i=0,16 do
	local e = ecs.CreateEntity()
	if i % 2 == 0 then
		local money = e:AddComponent "Money"
		money:Add(1000 + i * 10)
	end
	e:AddComponent "Position"
	if i % 3 ~= 0 then e:AddComponent("Velocity") end
end

