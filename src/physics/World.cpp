#include "physics/World.hpp"

#include <box2d/box2d.h>

#include "physics/Raycast.hpp"
#include "physics/Rigidbody.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sge::physics {

namespace {

class NullContactListener : public b2ContactListener {
    void BeginContact(b2Contact* /*contact*/) override {}
    void EndContact(b2Contact* /*contact*/) override {}
};

class RaycastReporter : public b2RayCastCallback {
public:
    using callback_t = std::function<void(b2Fixture*, const b2Vec2 &, const b2Vec2 &, float)>;

    RaycastReporter(callback_t &&callback)
        : callback_(std::move(callback)) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2 &point, const b2Vec2 &normal,
                        float fraction) override {
        std::invoke(this->callback_, fixture, point, normal, fraction);
        return 1;
    }

private:
    callback_t callback_;
};

} // namespace

World::World(b2ContactListener* contactListener)
    : contactListener_(contactListener)
    , nullListener_(std::make_unique<NullContactListener>()) {}

World::~World() {}

std::unique_ptr<Rigidbody> World::newRigidbody() {
    this->initializeWorldOnce();
    return std::make_unique<Rigidbody>(this->world_.get());
}

void World::step() {
    if (this->world_ == nullptr) {
        return;
    }

    constexpr auto TimeStep = 1.0F / 60.0F;
    constexpr auto VelocityIterations = 8;
    constexpr auto PositionIterations = 3;
    this->world_->Step(TimeStep, VelocityIterations, PositionIterations);
}

void World::enableCollisionReporting() {
    if (!this->world_) {
        return;
    }
    this->world_->SetContactListener(this->contactListener_);
}

void World::disableCollisionReporting() {
    if (!this->world_) {
        return;
    }
    this->world_->SetContactListener(this->nullListener_.get());
}

void World::initializeWorldOnce() {
    if (this->world_) {
        return;
    }
    this->world_ = std::make_unique<b2World>(b2Vec2{0.0F, 9.8F});
    this->enableCollisionReporting();
}

std::optional<HitResult> World::raycast(const b2Vec2 &pos, const b2Vec2 &direction,
                                        float distance) const {
    auto hits = this->raycastAll(pos, direction, distance);
    if (hits.empty()) {
        return std::nullopt;
    }
    return {hits.front()};
}

std::vector<HitResult> World::raycastAll(const b2Vec2 &pos, const b2Vec2 &direction,
                                         float distance) const {
    std::vector<HitResult> hits;

    if (!this->world_) {
        return hits;
    }

    auto reporter = RaycastReporter{
        [&](b2Fixture* fixture, const b2Vec2 &point, const b2Vec2 &normal, float fraction) -> void {
            auto* actor = ActorPointerOfFixture(fixture);
            hits.push_back(HitResult{actor, point, normal, fixture->IsSensor(), fraction});
        }};

    // Compute end point
    auto normalizedDirection = direction;
    normalizedDirection.Normalize();
    auto end = pos + (distance * normalizedDirection);
    // Raycast from start to end point
    this->world_->RayCast(&reporter, pos, end);

    // Sort by distance
    std::sort(hits.begin(), hits.end(), [](const auto &a, const auto &b) {
        return a.fraction < b.fraction;
    });

    return hits;
}

} // namespace sge::physics
