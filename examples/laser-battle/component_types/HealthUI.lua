HealthUI = {}

function HealthUI:OnStart()
    self.health_component = self.actor:GetComponent("PlayerHealth")
end

function HealthUI:OnUpdate()
    Text.Draw("Health: " .. tostring(self.health_component.health), 0, 0, "NotoSans-Regular", 36, 0, 0, 0, 255)
end
