#pragma once

#include <box2d/box2d.h>

#include "physics/Raycast.hpp"
#include "physics/Rigidbody.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace sge::physics {

inline game::Actor* ActorPointerOfFixture(b2Fixture* fixture) {
    // NOLINTBEGIN(performance-no-int-to-ptr)
    return reinterpret_cast<game::Actor*>(fixture->GetUserData().pointer);
    // NOLINTEND(performance-no-int-to-ptr)
}

class World {
public:
    World(b2ContactListener* contactListener);
    ~World();

    void step();
    void enableCollisionReporting();
    void disableCollisionReporting();

    std::optional<HitResult> raycast(const b2Vec2 &pos, const b2Vec2 &direction,
                                     float distance) const;
    std::vector<HitResult> raycastAll(const b2Vec2 &pos, const b2Vec2 &direction,
                                      float distance) const;

    std::unique_ptr<Rigidbody> newRigidbody();

private:
    void initializeWorldOnce();

    b2ContactListener* contactListener_;
    std::unique_ptr<b2ContactListener> nullListener_;
    std::unique_ptr<b2World> world_{nullptr};
};

} // namespace sge::physics
