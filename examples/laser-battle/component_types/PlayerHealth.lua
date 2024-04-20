PlayerHealth = {
    health = 10
}

function PlayerHealth:ReplicatePush(push)
    push:WriteInt(self.health)
end

function PlayerHealth:ReplicatePull(pull)
    self.health = pull:ReadInt()
end
