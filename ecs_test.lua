-- local CMoney = {}
-- CMoney.__index = CMoney
-- CMoney.balance = 0

-- function CMoney:Add(amt)
-- 	if self.balance + amt < 0 then
-- 		throw_error("Insufficient funds")
-- 		return
-- 	end
-- 	self.balance = self.balance + amt
-- end

-- ecs.RegisterComponent(CMoney, "Money")

for i=1,2 do
	local e = ecs.CreateEntity()
	print(e:GetId())
	e:AddComponent "Position"
end

-- local money = e:AddComponent("Money")
-- print(money)

-- money:Add(100)
-- print(money.balance)
