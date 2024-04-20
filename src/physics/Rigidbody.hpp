#pragma once

#include <box2d/box2d.h>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/components/CppComponent.hpp"
#include "util/B2Ptr.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::physics {

using namespace util;

// NOLINTBEGIN(readability-identifier-naming)

class Rigidbody : public scripting::CppComponent {
public:
    Rigidbody(b2World* world);

    const luabridge::LuaRef &ref() const override;

    std::unique_ptr<Component> clone() const override;
    void initialize() override;
    void setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) override;

    b2Vec2 GetPosition();
    float GetRotation();
    b2Vec2 GetVelocity();
    float GetAngularVelocity();
    float GetGravityScale();
    b2Vec2 GetUpDirection();
    b2Vec2 GetRightDirection();

    void AddForce(const b2Vec2 &f);
    void SetVelocity(const b2Vec2 &v);
    void SetPosition(const b2Vec2 &position);
    void SetRotation(float degreesClockwise);
    void SetAngularVelocity(float w);
    void SetGravityScale(float s);
    void SetUpDirection(b2Vec2 dir);
    void SetRightDirection(b2Vec2 dir);

    scripting::OpaqueComponentPointer __opaquePointer;

    float x{0.0F}, y{0.0F};
    std::string body_type{"dynamic"};
    bool precise{true};
    float gravity_scale{1.0F};
    float density{1.0F};
    float angular_friction{0.3F};
    float rotation{0.0F};

    bool has_collider{true};
    std::string collider_type{"box"};
    float width{1.0F}, height{1.0F};
    float radius{0.5F};
    float friction{0.3F};
    float bounciness{0.3F};

    bool has_trigger{true};
    std::string trigger_type{"box"};
    float trigger_width{1.0F}, trigger_height{1.0F};
    float trigger_radius{0.5F};

private:
    void initializeColliderFixture();
    void initializeTriggerFixture();
    void initializeDefaultFixture();

    luabridge::LuaRef ref_;
    b2World* world_;
    b2BodyPtr body_{nullptr};
};

// NOLINTEND(readability-identifier-naming)

} // namespace sge::physics
