LobbyManager = {
    lobby_size = 2
}

function LobbyManager:OnStart()
    Debug.Log("LobbyManager starting")
    self:UpdateJoinedPlayers()
    self.join_handle_1 = Multiplayer.OnClientJoin(function()
        self:UpdateJoinedPlayers()
    end)
    self.join_handle_2 = Multiplayer.OnClientLeave(function()
        self:UpdateJoinedPlayers()
    end)
end

function LobbyManager:UpdateJoinedPlayers()
    local joined = Multiplayer.JoinedClients()
    Debug.Log("Number of connected players = " .. #joined)
    if #joined >= self.lobby_size then
        Debug.Log("Enough players have connected")
        Scene.Load("battle")
    end
end

function LobbyManager:OnDestroy()
    Event.Unsubscribe(self.join_handle_1)
    Event.Unsubscribe(self.join_handle_2)
end
