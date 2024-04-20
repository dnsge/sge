#pragma once

#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "physics/Collision.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sge::scripting {

class LuaComponent : public Component {
public:
    LuaComponent(const ComponentType &baseType, Realm realm);
    LuaComponent(const LuaComponent &parent, Realm realm);

    LuaComponent &operator=(const LuaComponent &other) = delete;
    LuaComponent(LuaComponent &&inherited) noexcept = default;
    LuaComponent &operator=(LuaComponent &&inherited) noexcept = default;

    std::unique_ptr<Component> clone() const override;

    const luabridge::LuaRef &ref() const override;

    void initialize() override;

    void setActor(game::Actor* actor) override;
    void setKey(const std::string &key) override;
    void setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) override;

    bool getEnabled() const override;
    void setEnabled(bool enabled) override;

    void onStart() override;
    void onUpdate(float dt) override;
    void onLateUpdate(float dt) override;
    void onDestroy() override;

    void onCollisionEnter(const physics::Collision &collision) override;
    void onCollisionExit(const physics::Collision &collision) override;
    void onTriggerEnter(const physics::Collision &collision) override;
    void onTriggerExit(const physics::Collision &collision) override;

    void replicatePush(net::ReplicatePush &push) override;
    void replicatePull(net::ReplicatePull &pull) override;

private:
    luabridge::LuaRef ref_;
    std::optional<luabridge::LuaRef> onStart_ = std::nullopt;
    std::optional<luabridge::LuaRef> onUpdate_ = std::nullopt;
    std::optional<luabridge::LuaRef> onLateUpdate_ = std::nullopt;
    std::optional<luabridge::LuaRef> onDestroy_ = std::nullopt;

    std::optional<luabridge::LuaRef> onCollisionEnter_ = std::nullopt;
    std::optional<luabridge::LuaRef> onCollisionExit_ = std::nullopt;
    std::optional<luabridge::LuaRef> onTriggerEnter_ = std::nullopt;
    std::optional<luabridge::LuaRef> onTriggerExit_ = std::nullopt;

    std::optional<luabridge::LuaRef> replicatePush_ = std::nullopt;
    std::optional<luabridge::LuaRef> replicatePull_ = std::nullopt;
};

} // namespace sge::scripting
