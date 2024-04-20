LobbyStatus = {}

function LobbyStatus:OnStart()
    self.lobby_size = self.actor:GetComponent("LobbyManager").lobby_size
end

function LobbyStatus:OnUpdate()
    local connected_players = #Multiplayer.JoinedClients()
    local text = "Waiting for players... " .. connected_players .. "/" .. self.lobby_size
    Text.Draw(text, 0, 0, "NotoSans-Regular", 36, 0, 0, 0, 255)
end
