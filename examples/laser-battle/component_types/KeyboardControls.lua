KeyboardControls = {
    velocity = 1,
    transform_component = "Transform",
    replicate_transform = false
}

function KeyboardControls:OnStart()
    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
end

function KeyboardControls:OnUpdate(dt)
    local dir = KeyboardControls.GetDirectionVector()
    if dir:Length() == 0 then
        return
    end

    dir:Normalize()
    local delta = dir * (self.velocity * dt)

    self.t.x = self.t.x + delta.x
    self.t.y = self.t.y + delta.y

    if self.replicate_transform then
        ReplicatorService.Replicate(self.t)
    end
end

function KeyboardControls.GetDirectionVector()
    local dir = Vector2(0, 0)
    if Input.GetKey("up") or Input.GetKey("w") then
        dir.y = dir.y - 1
    end
    if Input.GetKey("down") or Input.GetKey("s") then
        dir.y = dir.y + 1
    end
    if Input.GetKey("left") or Input.GetKey("a") then
        dir.x = dir.x - 1
    end
    if Input.GetKey("right") or Input.GetKey("d") then
        dir.x = dir.x + 1
    end
    return dir
end
