#pragma once

#include <box2d/box2d.h>

#include <tuple>

namespace sge {

namespace game {
class Actor;
}

namespace physics {

enum class CollisionKind {
    Collider,
    Trigger
};

struct Collision {
    game::Actor* other;
    b2Vec2 point;
    b2Vec2 relative_velocity;
    b2Vec2 normal;
};

struct ActorCollision {
    game::Actor* me;
    Collision collision;
};

std::tuple<ActorCollision, ActorCollision, CollisionKind> CollisionFromContactEnter(
    b2Contact* contact);
std::tuple<ActorCollision, ActorCollision, CollisionKind> CollisionFromContactExit(
    b2Contact* contact);

} // namespace physics

} // namespace sge
