ConstantVelocity = {
    vel_x = 1,
    vel_y = 1,
    transform_component = "transform",
    replicate_transform = false
}

function ConstantVelocity:OnStart()
    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
end

function ConstantVelocity:OnUpdate(dt)
    self.t.x = self.t.x + self.vel_x * dt
    self.t.y = self.t.y + self.vel_y * dt
    if self.replicate_transform then
        ReplicatorService.Replicate(self.t)
    end
end
