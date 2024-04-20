ArenaDestroyer = {
    arena_actor_name = "???",
    transform_component = "transform"
}

function ArenaDestroyer:OnStart()
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

function ArenaDestroyer:OnUpdate(dt)
    if self.arena_component == nil then
        return
    end

    local arena_left = self.arena_component.x - self.arena_component.width / 2
    local arena_right = arena_left + self.arena_component.width
    local arena_top = self.arena_component.y - self.arena_component.height / 2
    local arena_bottom = arena_top + self.arena_component.height

    if self.t.x < arena_left or self.t.x > arena_right or self.t.y < arena_top or self.t.y > arena_bottom then
        Actor.Destroy(self.actor)
    end
end
