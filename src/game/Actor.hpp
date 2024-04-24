#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Types.hpp"
#include "physics/Collision.hpp"
#include "resources/Resources.hpp"
#include "scripting/ComponentContainer.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sge {

namespace scripting {
class Component;
}

namespace game {

enum class ActorLifecycleState {
    Uninitialized,
    Alive,
    PendingServerDestroy,
    Destroyed,
};

class Actor {
public:
    Actor(actor_id_t id, bool runtime, const resources::ActorDescription &source);

    actor_id_t id;
    std::optional<actor_id_t> remoteID{std::nullopt};
    std::optional<client_id_t> ownerClient{std::nullopt};

    std::string_view name{};
    scripting::ComponentContainer components{};
    ActorLifecycleState lifecycleState{ActorLifecycleState::Uninitialized};
    bool persistent{false};
    bool deferServerDestroys{false};

    bool destroyed() const;
    bool runtime() const;
    const std::string &runtimeTemplate();

    // ===================
    // Lifecycle functions
    // ===================
    bool runLifecycleFunctions() const;
    void onStart();
    void onUpdate(float dt);
    void onLateUpdate(float dt);
    void onDestroy();
    void onCollisionEnter(const physics::Collision &collision);
    void onCollisionExit(const physics::Collision &collision);
    void onTriggerEnter(const physics::Collision &collision);
    void onTriggerExit(const physics::Collision &collision);

    // ===================
    // Lua API
    // ===================
    std::string_view getName() const;
    actor_id_t getID() const;
    const std::optional<client_id_t> &getOwnerClient() const;

    scripting::Component* getComponentByKey(std::string_view key);
    scripting::Component* getComponent(std::string_view type);
    std::vector<scripting::Component*> getComponents(std::string_view type);

    scripting::Component* addComponent(std::string_view type);
    void removeComponent(const luabridge::LuaRef &ref);

    bool pendingServerDestroy() const;
    void destroy();
    void destroyLocally();
    void serverRequestedDestroy();

private:
    std::string runtimeTemplate_{};
};

} // namespace game

} // namespace sge

template <>
struct std::less<const sge::game::Actor*> {
    bool operator()(const sge::game::Actor* a1, const sge::game::Actor* a2) const {
        return a1->id < a2->id;
    }
};

template <>
struct std::less<sge::game::Actor*> {
    bool operator()(sge::game::Actor* a1, sge::game::Actor* a2) const {
        return std::less<const sge::game::Actor*>{}(a1, a2);
    }
};

template <>
struct std::greater<const sge::game::Actor*> {
    bool operator()(const sge::game::Actor* a1, const sge::game::Actor* a2) const {
        return a1->id > a2->id;
    }
};

template <>
struct std::greater<sge::game::Actor*> {
    bool operator()(sge::game::Actor* a1, sge::game::Actor* a2) const {
        return std::greater<const sge::game::Actor*>{}(a1, a2);
    }
};

template <>
struct std::equal_to<const sge::game::Actor*> {
    bool operator()(const sge::game::Actor* a1, const sge::game::Actor* a2) const {
        return a1->id == a2->id;
    }
};

template <>
struct std::equal_to<sge::game::Actor*> {
    bool operator()(sge::game::Actor* a1, sge::game::Actor* a2) const {
        return std::equal_to<const sge::game::Actor*>{}(a1, a2);
    }
};
