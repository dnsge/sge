BattleManager = {
    starting_radial_distance = 2.5,
    starting_health = 10
}

function BattleManager:OnStart()
    self.battle_state = self.actor:GetComponent("BattleState")

    local players = Multiplayer.JoinedClients()
    local count = #players
    local radial_spacing = 2 * math.pi / count

    for i, client_id in ipairs(players) do
        local ship = Actor.InstantiateOwned("BattleShip", client_id)
        local ship_t = ship:GetComponentByKey("transform")
        local player_health = ship:GetComponent("PlayerHealth")

        ship_t.x = math.cos((i - 1) * radial_spacing) * self.starting_radial_distance
        ship_t.y = math.sin((i - 1) * radial_spacing) * self.starting_radial_distance
        player_health.health = self.starting_health
    end

    self.join_handle = Multiplayer.OnClientJoin(function(client_id)
        Actor.InstantiateOwned("Spectator", client_id)
    end)

    self.leave_handle = Multiplayer.OnClientLeave(function()
        Scene.Load("lobby")
    end)
end

function BattleManager:OnDestroy()
    Event.Unsubscribe(self.join_handle)
    Event.Unsubscribe(self.leave_handle)
end
