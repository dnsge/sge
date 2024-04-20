#include "scripting/Component.hpp"

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Common.hpp"
#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "physics/Collision.hpp"
#include "resources/Deserialize.hpp"
#include "resources/Resources.hpp"
#include "scripting/Scripting.hpp"
#include "scripting/components/InterpTransform.hpp"
#include "scripting/components/LuaComponent.hpp"
#include "scripting/components/Transform.hpp"
#include "util/HeterogeneousLookup.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sge::scripting {

namespace {

size_t RuntimeComponentCounter = 0;

ComponentType loadComponentType(const std::filesystem::path &path, const std::string &name) {
    assert(std::filesystem::exists(path));
    auto* state = GetGlobalState();

    // Execute lua file to set global variable defining component
    if (luaL_dofile(state, path.string().c_str()) != LUA_OK) {
        std::cout << "problem with lua file " << path.stem().string() << std::endl;
        std::exit(0);
    }

    // Get reference to created component
    auto componentRef = luabridge::LuaRef::getGlobal(state, name.c_str());
    if (!componentRef.isTable()) {
        std::cout << "lua file " << path.string() << " does not define component table with name "
                  << name << std::endl;
        std::exit(0);
    }

    return ComponentType{componentRef, name};
}

} // namespace

unordered_string_map<ComponentType> ComponentTypes;

void InitializeComponentTypes() {
    if (!std::filesystem::exists(resources::ComponentTypesPath)) {
        // No component_types directory, so no custom lua components
        return;
    }

    auto it = std::filesystem::directory_iterator{resources::ComponentTypesPath};
    for (const auto &entry : it) {
        auto name = entry.path().stem().string();
        // Load component from file
        ComponentTypes.insert({name, loadComponentType(entry.path(), name)});
    }
}

std::string NextRuntimeComponentKey() {
    std::stringstream ss;
    ss << 'r' << RuntimeComponentCounter;
    ++RuntimeComponentCounter;
    return ss.str();
}

Component::Component(std::string type, Realm realm)
    : type(std::move(type))
    , realm(realm) {
    switch (CurrentRealm()) {
    case GeneralRealm::Server:
        this->realmMatches_ = this->realm == Realm::Server;
        break;
    case GeneralRealm::Client:
        this->realmMatches_ = this->realm == Realm::Client ||
                              this->realm == Realm::ServerReplicated || this->realm == Realm::Owner;
        break;
    }
}

void Component::setActor(game::Actor* actor) {
    this->actor = actor;
}

void Component::setKey(const std::string &key) {
    this->key = key;
}

void Component::setValues(
    const std::vector<std::pair<std::string, ComponentValueType>> & /*unused*/) {}

bool Component::initialized() const {
    return this->initialized_;
}

void Component::initialize() {
    assert(!this->initialized_);
    this->initialized_ = true;
}

bool Component::lifecycleShouldRun() const {
    return this->initialized() && this->realmMatches_ && this->getEnabled();
}

bool Component::lifecycleCanRunUnderActor() const {
    if (GameOffline()) {
        return true;
    }

    if (this->realm == Realm::Owner) {
        if (this->actor == nullptr) {
            return false;
        }
        if (!this->actor->ownerClient.has_value() ||
            CurrentClientID() != *this->actor->ownerClient) {
            return false;
        }
    }
    return true;
}

void Component::onStart() {}

void Component::onUpdate(float dt) {}

void Component::onLateUpdate(float dt) {}

void Component::onDestroy() {}

void Component::onCollisionEnter(const physics::Collision & /*unused*/) {}

void Component::onCollisionExit(const physics::Collision & /*unused*/) {}

void Component::onTriggerEnter(const physics::Collision & /*unused*/) {}

void Component::onTriggerExit(const physics::Collision & /*unused*/) {}

void Component::replicatePush(net::ReplicatePush & /*unused*/) {}

void Component::replicatePull(net::ReplicatePull & /*unused*/) {}

Component* RefToComponent(const luabridge::LuaRef &ref) {
    if (!ref.isTable() && !ref.isUserdata()) {
        throw std::runtime_error("tried to interpret non-component as component");
    }
    auto keyRef = ref[OpaqueComponentPointerKey];
    if (keyRef.isNil()) {
        throw std::runtime_error("tried to interpret non-component as component");
    }
    return keyRef.cast<OpaqueComponentPointer>().ptr;
}

std::unique_ptr<Component> InstantiateComponent(std::string_view type, Realm realm) {
    if (type == "Rigidbody") {
        return CurrentGame().physicsWorld().newRigidbody();
    } else if (type == "Transform") {
        return std::make_unique<Transform>(realm);
    } else if (type == "InterpTransform") {
        return std::make_unique<InterpTransform>(realm);
    }

    auto it = ComponentTypes.find(type);
    if (it == ComponentTypes.end()) {
        std::cout << "error: failed to locate component " << type;
        std::exit(0);
    }

    // Instantiate lua component
    return std::make_unique<LuaComponent>(it->second, realm);
}

} // namespace sge::scripting
