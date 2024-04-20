ArenaTrap = {
    arena_actor_name = "???",
    transform_component = "transform"
}

function ArenaTrap:OnStart()
    self.arena_actor = Actor.Find(self.arena_actor_name)
    if self.arena_actor == nil then
        error("Didn't find arena actor")
        return
    end

    self.arena_component = self.arena_actor:GetComponent("Arena")
    if self.arena_component == nil then
        error("Didn't find Arena component on arena actor")
        return
    end

    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
end

function ArenaTrap:OnUpdate(dt)
    if self.arena_component == nil then
        return
    end

    local arena_left = self.arena_component.x - self.arena_component.width / 2
    local arena_right = arena_left + self.arena_component.width
    local arena_top = self.arena_component.y - self.arena_component.height / 2
    local arena_bottom = arena_top + self.arena_component.height

    if self.t.x < arena_left then
        self.t.x = arena_left
    elseif self.t.x > arena_right then
        self.t.x = arena_right
    end
    if self.t.y < arena_top then
        self.t.y = arena_top
    elseif self.t.y > arena_bottom then
        self.t.y = arena_bottom
    end
end
