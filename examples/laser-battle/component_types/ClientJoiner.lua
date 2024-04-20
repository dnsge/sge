ClientJoiner = {}

function ClientJoiner:OnStart()
    self.connecting = false
end

function ClientJoiner:OnUpdate()
    if self.connecting then
        Text.Draw("Connecting to game...", 0, 0, "NotoSans-Regular", 12, 0, 0, 0, 255)
    else
        Text.Draw("Press [ENTER] to join", 0, 0, "NotoSans-Regular", 12, 0, 0, 0, 255)
    end

    if Input.GetKeyDown("enter") then
        self.connecting = true
        Multiplayer.Connect("localhost", "7462")
    end
end
