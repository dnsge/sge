#include "physics/Rigidbody.hpp"

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Realm.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/Scripting.hpp"
#include "scripting/components/CppComponent.hpp"
#include "util/B2Ptr.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::physics {

namespace {

inline float radiansOfDegrees(float degrees) {
    return degrees * (b2_pi / 180.0F);
}

inline float degreesOfRadians(float radians) {
    return radians * (180.0F / b2_pi);
}

constexpr int16_t CategoryCollider = 1 << 0;
constexpr int16_t CategoryTrigger = 1 << 1;

} // namespace

using namespace scripting;
using namespace util;

Rigidbody::Rigidbody(b2World* world)
    // TODO: Physics only on server?
    : CppComponent("Rigidbody", Realm::Server)
    , __opaquePointer{this}
    , ref_{GetGlobalState(), this}
    , world_(world) {}

const luabridge::LuaRef &Rigidbody::ref() const {
    return this->ref_;
}

void Rigidbody::initialize() {
    Component::initialize();

    b2BodyDef bodyDef;
    if (this->body_type == "dynamic") {
        bodyDef.type = b2_dynamicBody;
    } else if (this->body_type == "kinematic") {
        bodyDef.type = b2_kinematicBody;
    } else if (this->body_type == "static") {
        bodyDef.type = b2_staticBody;
    }

    bodyDef.position = b2Vec2{this->x, this->y};
    bodyDef.bullet = this->precise;
    bodyDef.gravityScale = this->gravity_scale;
    bodyDef.angularDamping = this->angular_friction;
    bodyDef.angle = radiansOfDegrees(this->rotation);

    this->body_ = b2BodyPtr{this->world_->CreateBody(&bodyDef), B2BodyDestroyer{this->world_}};

    // Set up fixtures
    if (this->has_collider) {
        this->initializeColliderFixture();
    }
    if (this->has_trigger) {
        this->initializeTriggerFixture();
    }
    if (!this->has_collider && !this->has_trigger) {
        this->initializeDefaultFixture();
    }
}

std::unique_ptr<Component> Rigidbody::clone() const {
    auto newRb = std::make_unique<Rigidbody>(this->world_);
    // General properties
    newRb->x = this->x;
    newRb->y = this->y;
    newRb->body_type = this->body_type;
    newRb->precise = this->precise;
    newRb->gravity_scale = this->gravity_scale;
    newRb->density = this->density;
    newRb->angular_friction = this->angular_friction;
    newRb->rotation = this->rotation;
    // Collider properties
    newRb->has_collider = this->has_collider;
    newRb->collider_type = this->collider_type;
    newRb->width = this->width;
    newRb->height = this->height;
    newRb->radius = this->radius;
    newRb->friction = this->friction;
    newRb->bounciness = this->bounciness;
    // Trigger properties
    newRb->has_trigger = this->has_trigger;
    newRb->trigger_type = this->trigger_type;
    newRb->trigger_width = this->trigger_width;
    newRb->trigger_height = this->trigger_height;
    newRb->trigger_radius = this->trigger_radius;

    assert(!newRb->initialized());
    return newRb;
}

void Rigidbody::setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) {
    for (const auto &[name, val] : values) {
        if (name == "x") {
            this->x = MustGet<float>(val);
        } else if (name == "y") {
            this->y = MustGet<float>(val);
        } else if (name == "body_type") {
            this->body_type = MustGet<std::string>(val);
        } else if (name == "precise") {
            this->precise = MustGet<bool>(val);
        } else if (name == "gravity_scale") {
            this->gravity_scale = MustGet<float>(val);
        } else if (name == "density") {
            this->density = MustGet<float>(val);
        } else if (name == "angular_friction") {
            this->angular_friction = MustGet<float>(val);
        } else if (name == "rotation") {
            this->rotation = MustGet<float>(val);
        }
        // Collider properties
        else if (name == "has_collider") {
            this->has_collider = MustGet<bool>(val);
        } else if (name == "collider_type") {
            this->collider_type = MustGet<std::string>(val);
        } else if (name == "width") {
            this->width = MustGet<float>(val);
        } else if (name == "height") {
            this->height = MustGet<float>(val);
        } else if (name == "radius") {
            this->radius = MustGet<float>(val);
        } else if (name == "friction") {
            this->friction = MustGet<float>(val);
        } else if (name == "bounciness") {
            this->bounciness = MustGet<float>(val);
        }
        // Trigger properties
        else if (name == "has_trigger") {
            this->has_trigger = MustGet<bool>(val);
        } else if (name == "trigger_type") {
            this->trigger_type = MustGet<std::string>(val);
        } else if (name == "trigger_width") {
            this->trigger_width = MustGet<float>(val);
        } else if (name == "trigger_height") {
            this->trigger_height = MustGet<float>(val);
        } else if (name == "trigger_radius") {
            this->trigger_radius = MustGet<float>(val);
        }
    }
}

void Rigidbody::initializeColliderFixture() {
    assert(this->has_collider);

    b2Shape* shape = nullptr;
    b2PolygonShape polygonShape;
    b2CircleShape circleShape;
    if (this->collider_type == "box") {
        polygonShape.SetAsBox(0.5F * this->width, 0.5F * this->height);
        shape = &polygonShape;
    } else if (this->collider_type == "circle") {
        circleShape.m_radius = this->radius;
        shape = &circleShape;
    } else {
        std::cout << "invalid collider type";
        std::exit(0);
    }

    b2FixtureDef fixtureDef;
    fixtureDef.shape = shape;
    fixtureDef.density = this->density;
    fixtureDef.friction = this->friction;
    fixtureDef.restitution = this->bounciness;
    fixtureDef.isSensor = false;

    // Filter contact within same category
    fixtureDef.filter.categoryBits = CategoryCollider;
    fixtureDef.filter.maskBits = CategoryCollider;

    // Store pointer to actor
    fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);

    this->body_->CreateFixture(&fixtureDef);
}

void Rigidbody::initializeTriggerFixture() {
    assert(this->has_trigger);

    b2Shape* shape = nullptr;
    b2PolygonShape polygonShape;
    b2CircleShape circleShape;
    if (this->trigger_type == "box") {
        polygonShape.SetAsBox(0.5F * this->trigger_width, 0.5F * this->trigger_height);
        shape = &polygonShape;
    } else if (this->trigger_type == "circle") {
        circleShape.m_radius = this->trigger_radius;
        shape = &circleShape;
    } else {
        std::cout << "invalid trigger type";
        std::exit(0);
    }

    b2FixtureDef fixtureDef;
    fixtureDef.shape = shape;
    fixtureDef.density = this->density;
    fixtureDef.isSensor = true;

    // Filter contact within same category
    fixtureDef.filter.categoryBits = CategoryTrigger;
    fixtureDef.filter.maskBits = CategoryTrigger;

    // Store pointer to actor
    fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);

    this->body_->CreateFixture(&fixtureDef);
}

void Rigidbody::initializeDefaultFixture() {
    b2PolygonShape shape;
    shape.SetAsBox(0.5F * this->width, 0.5F * this->height);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = this->density;
    fixtureDef.isSensor = true;
    fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);

    this->body_->CreateFixture(&fixtureDef);
}

b2Vec2 Rigidbody::GetPosition() {
    if (this->body_ == nullptr) {
        return b2Vec2{this->x, this->y};
    }

    return this->body_->GetPosition();
}

float Rigidbody::GetRotation() {
    if (this->body_ == nullptr) {
        return this->rotation;
    }

    auto radians = this->body_->GetAngle();
    return degreesOfRadians(radians);
}

b2Vec2 Rigidbody::GetVelocity() {
    if (this->body_ == nullptr) {
        return b2Vec2{0.0F, 0.0F};
    }

    return this->body_->GetLinearVelocity();
}

float Rigidbody::GetAngularVelocity() {
    if (this->body_ == nullptr) {
        return 0.0F;
    }

    auto wRadians = this->body_->GetAngularVelocity();
    return degreesOfRadians(wRadians);
}

float Rigidbody::GetGravityScale() {
    if (this->body_ == nullptr) {
        return this->gravity_scale;
    }

    return this->body_->GetGravityScale();
}

b2Vec2 Rigidbody::GetUpDirection() {
    if (this->body_ == nullptr) {
        return b2Vec2{0, -1};
    }

    auto angleRadians = this->body_->GetAngle();
    // Note: rotated by -90º
    auto x = glm::sin(angleRadians);
    auto y = -glm::cos(angleRadians);
    return b2Vec2{x, y};
}

b2Vec2 Rigidbody::GetRightDirection() {
    if (this->body_ == nullptr) {
        return b2Vec2{1, 0};
    }

    auto angleRadians = this->body_->GetAngle();
    auto x = glm::cos(angleRadians);
    auto y = glm::sin(angleRadians);
    return b2Vec2{x, y};
}

void Rigidbody::AddForce(const b2Vec2 &f) {
    if (this->body_ == nullptr) {
        return;
    }

    this->body_->ApplyForceToCenter(f, true);
}

void Rigidbody::SetVelocity(const b2Vec2 &v) {
    if (this->body_ == nullptr) {
        return;
    }

    this->body_->SetLinearVelocity(v);
}

void Rigidbody::SetPosition(const b2Vec2 &position) {
    if (this->body_ == nullptr) {
        this->x = position.x;
        this->y = position.y;
        return;
    }

    this->body_->SetTransform(position, this->body_->GetAngle());
}

void Rigidbody::SetRotation(float degreesClockwise) {
    if (this->body_ == nullptr) {
        this->rotation = degreesClockwise;
        return;
    }

    auto angleRadians = radiansOfDegrees(degreesClockwise);
    this->body_->SetTransform(this->body_->GetPosition(), angleRadians);
}

void Rigidbody::SetAngularVelocity(float w) {
    if (this->body_ == nullptr) {
        return;
    }

    auto wRadians = radiansOfDegrees(w);
    this->body_->SetAngularVelocity(wRadians);
}

void Rigidbody::SetGravityScale(float s) {
    if (this->body_ == nullptr) {
        this->gravity_scale = s;
        return;
    }

    this->body_->SetGravityScale(s);
}

void Rigidbody::SetUpDirection(b2Vec2 dir) {
    dir.Normalize();
    float angleRadians = glm::atan(dir.x, -dir.y);

    if (this->body_ == nullptr) {
        this->rotation = degreesOfRadians(angleRadians);
        return;
    }

    this->body_->SetTransform(this->body_->GetPosition(), angleRadians);
}

void Rigidbody::SetRightDirection(b2Vec2 dir) {
    dir.Normalize();
    float angleRadians = glm::atan(dir.x, -dir.y) - b2_pi / 2.0F;

    if (this->body_ == nullptr) {
        this->rotation = degreesOfRadians(angleRadians);
        return;
    }

    this->body_->SetTransform(this->body_->GetPosition(), angleRadians);
}

} // namespace sge::physics
