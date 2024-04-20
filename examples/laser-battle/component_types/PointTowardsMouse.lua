PointTowardsMouse = {
    transform_component = "transform",
    replicate_transform = false
}

function PointTowardsMouse:OnStart()
    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
end

function PointTowardsMouse:OnUpdate(dt)
    local mousePos = Input.GetMousePositionScene()
    local angle = math.deg(math.atan(mousePos.y - self.t.y, mousePos.x - self.t.x))

    if math.abs(angle - self.t.rotation) <= 2 then
        return
    end

    self.t.rotation = angle
    if self.replicate_transform then
        ReplicatorService.Replicate(self.t)
    end
end
