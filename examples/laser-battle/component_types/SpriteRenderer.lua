SpriteRenderer = {
	sprite = "???",
	r = 255,
	g = 255,
	b = 255,
	a = 255,
	x_offset = 0,
	y_offset = 0,
	rotation_offset = 0,
	sorting_order = 0,
	transform_component = "transform"
}

function SpriteRenderer:OnStart()
	self.t = self.actor:GetComponentByKey(self.transform_component)
	if self.t == nil then
		error("Didn't find transform_component on actor")
	end
end

function SpriteRenderer:OnUpdate(dt)
	local x = self.t.x + self.x_offset
	local y = self.t.y + self.y_offset
	local rot = self.t.rotation + self.rotation_offset
	Image.DrawEx(self.sprite, x, y, rot, 1.0, 1.0, 0.5, 0.5, self.r, self.g, self.b, self.a, self.sorting_order)
end
