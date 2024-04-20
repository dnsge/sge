#pragma once

#include <box2d/box2d.h>

namespace sge {

namespace game {
class Actor;
}

namespace physics {

struct HitResult {
    game::Actor* actor;
    b2Vec2 point;
    b2Vec2 normal;
    bool is_trigger;
    float fraction;
};

} // namespace physics

} // namespace sge
