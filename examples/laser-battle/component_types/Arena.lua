Arena = {
    x = 0,
    y = 0,
    width = 0,
    height = 0,
}

function Arena:ReplicatePush(push)
    push:WriteNumber(self.x)
    push:WriteNumber(self.y)
    push:WriteNumber(self.width)
    push:WriteNumber(self.height)
end

function Arena:ReplicatePull(pull)
    self.x = pull:ReadNumber()
    self.y = pull:ReadNumber()
    self.width = pull:ReadNumber()
    self.height = pull:ReadNumber()
end
