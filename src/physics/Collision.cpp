#include "physics/Collision.hpp"

#include <box2d/box2d.h>

#include "physics/World.hpp"

#include <tuple>

namespace sge::physics {

namespace {

const auto InvalidCollisionVec = b2Vec2{-999.0F, -999.0F};

std::tuple<game::Actor*, game::Actor*> getActorsFromContact(b2Contact* contact) {
    auto* actorA = ActorPointerOfFixture(contact->GetFixtureA());
    auto* actorB = ActorPointerOfFixture(contact->GetFixtureB());
    return std::make_tuple(actorA, actorB);
}

CollisionKind collisionKindOfContact(b2Contact* contact) {
    bool isSensor = contact->GetFixtureA()->IsSensor() || contact->GetFixtureB()->IsSensor();
    return isSensor ? CollisionKind::Trigger : CollisionKind::Collider;
}

} // namespace

std::tuple<ActorCollision, ActorCollision, CollisionKind> CollisionFromContactEnter(
    b2Contact* contact) {
    auto collisionClass = collisionKindOfContact(contact);
    const auto [actorA, actorB] = getActorsFromContact(contact);

    // Compute relative velocity
    const auto &aVel = contact->GetFixtureA()->GetBody()->GetLinearVelocity();
    const auto &bVel = contact->GetFixtureB()->GetBody()->GetLinearVelocity();
    auto relativeVelocity = aVel - bVel;

    // Default to sentinel value
    b2Vec2 collisionPoint = InvalidCollisionVec;
    b2Vec2 normal = InvalidCollisionVec;

    if (collisionClass == CollisionKind::Collider) {
        // Get first collision point
        b2WorldManifold worldManifold;
        contact->GetWorldManifold(&worldManifold);
        collisionPoint = worldManifold.points[0];
        normal = worldManifold.normal;
    }

    auto collisionA = ActorCollision{
        actorA, Collision{actorB, collisionPoint, relativeVelocity, normal}
    };
    auto collisionB = ActorCollision{
        actorB, Collision{actorA, collisionPoint, relativeVelocity, normal}
    };
    return std::make_tuple(collisionA, collisionB, collisionClass);
}

std::tuple<ActorCollision, ActorCollision, CollisionKind> CollisionFromContactExit(
    b2Contact* contact) {
    auto collisionClass = collisionKindOfContact(contact);
    const auto [actorA, actorB] = getActorsFromContact(contact);

    // Compute relative velocity
    const auto &aVel = contact->GetFixtureA()->GetBody()->GetLinearVelocity();
    const auto &bVel = contact->GetFixtureB()->GetBody()->GetLinearVelocity();
    auto relativeVelocity = aVel - bVel;

    auto collisionA = ActorCollision{
        actorA, Collision{actorB, InvalidCollisionVec, relativeVelocity, InvalidCollisionVec}
    };
    auto collisionB = ActorCollision{
        actorB, Collision{actorA, InvalidCollisionVec, relativeVelocity, InvalidCollisionVec}
    };
    return std::make_tuple(collisionA, collisionB, collisionClass);
}

} // namespace sge::physics
