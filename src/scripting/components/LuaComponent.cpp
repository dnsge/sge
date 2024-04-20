#include "scripting/components/LuaComponent.hpp"

#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "physics/Collision.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace sge::scripting {

namespace {

void establishInheritance(luabridge::LuaRef &child, const luabridge::LuaRef &parent) {
    assert(child.state() == parent.state());

    // Create metatable instance pointing to base component
    auto metatable = luabridge::newTable(child.state());
    metatable["__index"] = parent;

    // Set metatable of this component
    child.push(child.state());
    metatable.push(child.state());
    lua_setmetatable(child.state(), -2);
    lua_pop(child.state(), 1);
}

} // namespace

LuaComponent::LuaComponent(const ComponentType &baseType, Realm realm)
    : Component(baseType.name, realm)
    , ref_(luabridge::newTable(baseType.ref.state())) {
    // Establish inheritance of component
    establishInheritance(this->ref_, baseType.ref);
    // Store opaque pointer to this component
    this->ref_[OpaqueComponentPointerKey] = OpaqueComponentPointer{this};
}

LuaComponent::LuaComponent(const LuaComponent &parent, Realm realm)
    : Component(parent.type, realm)
    , ref_(luabridge::newTable(parent.ref_.state())) {
    // Establish inheritance of component
    establishInheritance(this->ref_, parent.ref_);
    // Store opaque pointer to this component
    this->ref_[OpaqueComponentPointerKey] = OpaqueComponentPointer{this};
}

std::unique_ptr<Component> LuaComponent::clone() const {
    // TODO: Accept realm as parameter?
    return std::make_unique<LuaComponent>(*this, this->realm);
}

const luabridge::LuaRef &LuaComponent::ref() const {
    return this->ref_;
}

void LuaComponent::initialize() {
    Component::initialize();
    if (auto ref = this->ref_["OnStart"]; ref.isFunction()) {
        this->onStart_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnUpdate"]; ref.isFunction()) {
        this->onUpdate_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnLateUpdate"]; ref.isFunction()) {
        this->onLateUpdate_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnDestroy"]; ref.isFunction()) {
        this->onDestroy_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnCollisionEnter"]; ref.isFunction()) {
        this->onCollisionEnter_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnCollisionExit"]; ref.isFunction()) {
        this->onCollisionExit_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnTriggerEnter"]; ref.isFunction()) {
        this->onTriggerEnter_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["OnTriggerExit"]; ref.isFunction()) {
        this->onTriggerExit_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["ReplicatePush"]; ref.isFunction()) {
        this->replicatePush_.emplace(std::move(ref));
    }
    if (auto ref = this->ref_["ReplicatePull"]; ref.isFunction()) {
        this->replicatePull_.emplace(std::move(ref));
    }
}

void LuaComponent::setActor(game::Actor* actor) {
    Component::setActor(actor);
    this->ref_["actor"] = actor;
}

void LuaComponent::setKey(const std::string &key) {
    Component::setKey(key);
    this->ref_["key"] = key;
}

void LuaComponent::setValues(
    const std::vector<std::pair<std::string, ComponentValueType>> &values) {
    for (const auto &entry : values) {
        const auto &name = entry.first;
        const auto &value = entry.second;
        if (std::holds_alternative<std::string>(value)) {
            this->ref_[name] = std::get<std::string>(value);
        } else if (std::holds_alternative<int>(value)) {
            this->ref_[name] = std::get<int>(value);
        } else if (std::holds_alternative<float>(value)) {
            this->ref_[name] = std::get<float>(value);
        } else if (std::holds_alternative<bool>(value)) {
            this->ref_[name] = std::get<bool>(value);
        }
    }
}

bool LuaComponent::getEnabled() const {
    assert(this->ref_.isTable());

    auto* l = this->ref_.state();
    this->ref_.push();                      // Push table onto stack
    lua_getfield(l, -1, "enabled");         // Access "enabled" key in table
    bool value = lua_toboolean(l, -1) != 0; // Convert value to boolean
    lua_pop(l, 2);                          // Clean up lua stack

    return value;
}

void LuaComponent::setEnabled(bool enabled) {
    this->ref_["enabled"] = enabled;
}

void LuaComponent::onStart() {
    if (this->onStart_.has_value()) {
        (*this->onStart_)(this->ref_);
    }
}

void LuaComponent::onUpdate(float dt) {
    if (this->onUpdate_.has_value()) {
        (*this->onUpdate_)(this->ref_, dt);
    }
}

void LuaComponent::onLateUpdate(float dt) {
    if (this->onLateUpdate_.has_value()) {
        (*this->onLateUpdate_)(this->ref_, dt);
    }
}

void LuaComponent::onDestroy() {
    if (this->onDestroy_.has_value()) {
        (*this->onDestroy_)(this->ref_);
    }
}

void LuaComponent::onCollisionEnter(const physics::Collision &collision) {
    if (this->onCollisionEnter_.has_value()) {
        (*this->onCollisionEnter_)(this->ref_, collision);
    }
}

void LuaComponent::onCollisionExit(const physics::Collision &collision) {
    if (this->onCollisionExit_.has_value()) {
        (*this->onCollisionExit_)(this->ref_, collision);
    }
}
void LuaComponent::onTriggerEnter(const physics::Collision &collision) {
    if (this->onTriggerEnter_.has_value()) {
        (*this->onTriggerEnter_)(this->ref_, collision);
    }
}

void LuaComponent::onTriggerExit(const physics::Collision &collision) {
    if (this->onTriggerExit_.has_value()) {
        (*this->onTriggerExit_)(this->ref_, collision);
    }
}

void LuaComponent::replicatePush(net::ReplicatePush &push) {
    if (this->replicatePush_.has_value()) {
        (*this->replicatePush_)(this->ref_, push);
    }
}

void LuaComponent::replicatePull(net::ReplicatePull &pull) {
    if (this->replicatePull_.has_value()) {
        (*this->replicatePull_)(this->ref_, pull);
    }
}

} // namespace sge::scripting
