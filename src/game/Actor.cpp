#include "game/Actor.hpp"

#include <glm/glm.hpp>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Realm.hpp"
#include "Types.hpp"
#include "physics/Collision.hpp"
#include "resources/Resources.hpp"
#include "scripting/ActorTemplate.hpp"
#include "scripting/Component.hpp"
#include "scripting/ComponentContainer.hpp"
#include "scripting/Invoke.hpp"

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::game {

using namespace sge::resources;
using namespace std::string_view_literals;

namespace {

template <typename Dst, typename Src>
void applyOverride(Dst &dst, const std::optional<Src> &src) {
    if (!src.has_value()) {
        return;
    }
    dst = {*src};
}

scripting::ComponentContainer constructComponentsForActor(const resources::ActorDescription &source,
                                                          Actor &actor) {
    std::map<std::string, std::unique_ptr<scripting::Component>, std::less<>> components;

    if (source.template_name) {
        const auto &instance = scripting::GetActorTemplateInstance(*source.template_name);
        // Create components based on template components
        for (const auto &component : instance.components()) {
            auto instance = component.second->clone();
            instance->setActor(&actor);
            instance->setKey(component.first);
            instance->setEnabled(true);
            components.emplace(component.first, std::move(instance));
        }
    }

    for (const auto &item : source.components) {
        auto existingIt = components.find(item.first);
        if (existingIt == components.end()) {
            // New component
            auto component = scripting::InstantiateComponent(item.second.type, item.second.realm);
            component->setActor(&actor);
            component->setKey(item.first);
            component->setValues(item.second.values);
            component->setEnabled(true);
            components.emplace(item.first, std::move(component));
            continue;
        }

        // Overriding component provided by template
        existingIt->second->setValues(item.second.values);
    }

    return scripting::ComponentContainer{std::move(components)};
}

template <class... Args, class... Args2>
void callComponentFunc(Actor &actor, void (scripting::Component::*componentFunc)(Args...),
                       Args2 &&... args) {
    for (auto &entry : actor.components) {
        if (!actor.runLifecycleFunctions()) {
            break;
        }

        auto &component = entry.second;
        if (!component->lifecycleCanRunUnderActor()) {
            // Component should only run on owner client
            continue;
        }
        if (component->lifecycleShouldRun()) {
            scripting::ActorInvoke(actor.name, [&]() {
                (component.get()->*componentFunc)(std::forward<Args2>(args)...);
            });
        }
    }

    // Clean up any removed components
    actor.components.removeDeferred();
}

} // namespace

Actor::Actor(actor_id_t id, bool runtime, const resources::ActorDescription &source)
    : id(id) {
    if (source.template_name) {
        // Initialize with template data
        const auto &actorTemplate = resources::GetActorTemplateDescription(*source.template_name);
        this->name = actorTemplate.name;
    } else {
        // Initialize with defaults
        this->name = ""sv;
    }

    if (runtime) {
        // Mark this actor as runtime-instantiated
        assert(source.template_name.has_value());
        this->runtimeTemplate_ = *source.template_name;
    }

    // Apply any POD overrides
    applyOverride(this->name, source.name);

    // Construct component instances
    this->components = constructComponentsForActor(source, *this);
}

bool Actor::destroyed() const {
    return this->lifecycleState == ActorLifecycleState::Destroyed;
}

bool Actor::runtime() const {
    return !this->runtimeTemplate_.empty();
}

const std::string &Actor::runtimeTemplate() {
    return this->runtimeTemplate_;
}

bool Actor::runLifecycleFunctions() const {
    return this->lifecycleState == ActorLifecycleState::Alive;
}

void Actor::onStart() {
    for (auto &entry : this->components) {
        if (!this->runLifecycleFunctions()) {
            break;
        }
        auto &component = entry.second;
        if (component->initialized()) {
            // Only initialize once
            continue;
        }
        if (!component->lifecycleCanRunUnderActor()) {
            // Component should only run on owner client
            continue;
        }
        // Mark component as initialized
        component->initialize();
        if (component->lifecycleShouldRun()) {
            scripting::ActorInvoke(this->name, [&]() {
                component->onStart();
            });
        }
    }
}

void Actor::onUpdate(float dt) {
    callComponentFunc(*this, &scripting::Component::onUpdate, dt);
}

void Actor::onLateUpdate(float dt) {
    callComponentFunc(*this, &scripting::Component::onLateUpdate, dt);
}

void Actor::onDestroy() {
    callComponentFunc(*this, &scripting::Component::onDestroy);
}

void Actor::onCollisionEnter(const physics::Collision &collision) {
    callComponentFunc(*this, &scripting::Component::onCollisionEnter, collision);
}

void Actor::onCollisionExit(const physics::Collision &collision) {
    callComponentFunc(*this, &scripting::Component::onCollisionExit, collision);
}

void Actor::onTriggerEnter(const physics::Collision &collision) {
    callComponentFunc(*this, &scripting::Component::onTriggerEnter, collision);
}

void Actor::onTriggerExit(const physics::Collision &collision) {
    callComponentFunc(*this, &scripting::Component::onTriggerExit, collision);
}

std::string_view Actor::getName() const {
    return this->name;
}

actor_id_t Actor::getID() const {
    return this->id;
}

const std::optional<client_id_t> &Actor::getOwnerClient() const {
    return this->ownerClient;
}

scripting::Component* Actor::getComponentByKey(std::string_view key) {
    return this->components.getComponentByKey(key);
}

scripting::Component* Actor::getComponent(std::string_view type) {
    return this->components.getComponent(type);
}

std::vector<scripting::Component*> Actor::getComponents(std::string_view type) {
    return this->components.getComponents(type);
}

scripting::Component* Actor::addComponent(std::string_view type) {
    // Add a component at runtime. Any components added to an actor after instantiation
    // will not be replicated, so assign the realm to match whether we are running
    // on the server or the client.
    auto realm = CurrentRealm() == GeneralRealm::Server ? Realm::Server : Realm::Client;
    auto instance = scripting::InstantiateComponent(std::string(type), realm);
    auto key = scripting::NextRuntimeComponentKey();
    instance->setActor(this);
    instance->setKey(key);
    instance->setEnabled(true);
    return this->components.addComponent(key, std::move(instance));
}

void Actor::removeComponent(const luabridge::LuaRef &ref) {
    auto* component = this->components.getComponent(ref);
    if (component == nullptr) {
        return;
    }
    component->setEnabled(false);
    this->components.removeComponentLater(component);
}

void Actor::destroy() {
    this->lifecycleState = ActorLifecycleState::Destroyed;
    CurrentReplicatorService().destroy(this);
}

void Actor::destroyLocally() {
    this->lifecycleState = ActorLifecycleState::Destroyed;
    CurrentReplicatorService().erasePendingReplications(this);
}

} // namespace sge::game
