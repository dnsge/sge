#pragma once

#include <box2d/box2d.h>

#include <memory>

namespace sge::util {

struct B2BodyDestroyer {
    b2World* world;
    void operator()(b2Body* b) const {
        world->DestroyBody(b);
    }
};

using b2BodyPtr = std::unique_ptr<b2Body, B2BodyDestroyer>;

} // namespace sge::util
