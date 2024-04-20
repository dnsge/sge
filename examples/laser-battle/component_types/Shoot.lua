Shoot = {
    keybind = "space",
    cooldown = 10,
    bullet_velocity = 10,

    transform_component = "transform",

    bullet_template = "???",
    bullet_transform_component = "transform",
}

function Shoot:OnStart()
    self.last_shot = Application.GetFrame()
    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
end

function Shoot:OnUpdate(dt)
    if Input.GetKey(self.keybind) then
        local now = Application.GetFrame()
        if now - self.last_shot >= self.cooldown then
            self:Shoot()
            self.last_shot = now
        end
    end
end

function Shoot:Shoot()
    local bullet = Actor.Instantiate(self.bullet_template)
    local bullet_t = bullet:GetComponentByKey(self.bullet_transform_component)
    local bullet_v = bullet:GetComponent("ConstantVelocity")

    -- Set bullet position
    bullet_t.x = self.t.x
    bullet_t.y = self.t.y
    bullet_t.rotation = self.t.rotation

    -- Set velocity of bullet
    local angle = math.rad(self.t.rotation)
    bullet_v.vel_x = self.bullet_velocity * math.cos(angle)
    bullet_v.vel_y = self.bullet_velocity * math.sin(angle)
end
