#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Realm.hpp"
#include "physics/Collision.hpp"
#include "resources/Deserialize.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge {

namespace game {
class Actor;
}

namespace net {
class ReplicatePush;
class ReplicatePull;
} // namespace net

namespace scripting {

struct ComponentType {
    luabridge::LuaRef ref;
    std::string name;
};

void InitializeComponentTypes();

std::string NextRuntimeComponentKey();

class Component {
public:
    Component(std::string type, Realm realm);
    virtual ~Component() = default;

    Component &operator=(const Component &other) = delete;
    Component(Component &&inherited) = default;
    Component &operator=(Component &&inherited) = default;

    virtual std::unique_ptr<Component> clone() const = 0;
    virtual const luabridge::LuaRef &ref() const = 0;

    virtual void setActor(game::Actor* actor);
    virtual void setKey(const std::string &key);
    virtual void setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values);

    bool initialized() const;
    virtual void initialize();

    bool lifecycleShouldRun() const;
    bool lifecycleCanRunUnderActor() const;

    virtual bool getEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;

    virtual void onStart();
    virtual void onUpdate(float dt);
    virtual void onLateUpdate(float dt);
    virtual void onDestroy();

    virtual void onCollisionEnter(const physics::Collision &collision);
    virtual void onCollisionExit(const physics::Collision &collision);
    virtual void onTriggerEnter(const physics::Collision &collision);
    virtual void onTriggerExit(const physics::Collision &collision);

    virtual void replicatePush(net::ReplicatePush &);
    virtual void replicatePull(net::ReplicatePull &);

    std::string type;
    Realm realm;
    game::Actor* actor = nullptr;
    std::string key{};

protected:
    bool initialized_ = false;
    bool realmMatches_ = false;
};

struct OpaqueComponentPointer {
    Component* ptr;
};

constexpr const char* OpaqueComponentPointerKey = "__opaque_component";

Component* RefToComponent(const luabridge::LuaRef &ref);

std::unique_ptr<Component> InstantiateComponent(std::string_view type, Realm realm);

} // namespace scripting

} // namespace sge

template <>
struct luabridge::Stack<sge::scripting::Component*> {
    static void push(lua_State* L, sge::scripting::Component* component) {
        if (component == nullptr) {
            luabridge::Stack<luabridge::Nil>::push(L, luabridge::Nil{});
            return;
        }
        luabridge::Stack<luabridge::LuaRef>::push(L, component->ref());
    }

    static sge::scripting::Component* get(lua_State* L, int index) {
        auto ref = luabridge::Stack<luabridge::LuaRef>::get(L, index);
        return sge::scripting::RefToComponent(ref);
    }

    static bool isInstance(lua_State* L, int index) {
        try {
            luabridge::Stack<sge::scripting::Component*>::get(L, index);
            return true;
        } catch (const std::runtime_error &) {
            return false;
        }
    }
};
