Damager = {
    transform_component = "transform",
    player_transform_component = "transform",
    damage_radius = 0.25
}

function Damager:OnStart()
    self.t = self.actor:GetComponentByKey(self.transform_component)
    if self.t == nil then
        error("Didn't find transform_component on actor")
    end
    self.player_actors = Actor.FindAll("battle_ship")
end

function Damager:OnUpdate()
    local this_owner = self.actor:GetOwner()

    for i, actor in ipairs(self.player_actors) do
        local actor_owner = actor:GetOwner()
        if actor_owner == this_owner then
            goto continue
        end

        local player_t = actor:GetComponentByKey(self.player_transform_component)
        local dx = math.abs(self.t.x - player_t.x)
        local dy = math.abs(self.t.y - player_t.y)
        local dist = math.sqrt(dx * dx + dy * dy)
        if dist < self.damage_radius then
            -- Decrement health
            local player_h = actor:GetComponent("PlayerHealth")
            player_h.health = player_h.health - 1
            ReplicatorService.Replicate(player_h)

            -- Publish event
            Event.PublishRemote("battle.PlayerTookDamage", {
                ["health"] = player_h.health,
                ["player"] = actor_owner,
            }, false)

            -- Destroy this bullet
            Actor.Destroy(self.actor)
            return
        end

        ::continue::
    end
end
