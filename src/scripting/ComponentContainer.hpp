#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "scripting/Component.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sge::scripting {

class ComponentContainer {
public:
    ComponentContainer() = default;
    ComponentContainer(std::map<std::string, std::unique_ptr<Component>, std::less<>> &&components);
    ~ComponentContainer();

    ComponentContainer(const ComponentContainer &) = delete;
    ComponentContainer &operator=(const ComponentContainer &) = delete;
    ComponentContainer(ComponentContainer &&) = default;
    ComponentContainer &operator=(ComponentContainer &&) = default;

    Component* addComponent(std::string_view key, std::unique_ptr<Component> &&component);
    void removeComponent(std::string_view key);
    void removeComponent(Component* component);
    void removeComponentLater(Component* component);
    void removeDeferred();

    Component* getComponentByKey(std::string_view key);
    Component* getComponent(std::string_view type);
    Component* getComponent(const luabridge::LuaRef &componentRef);
    std::vector<Component*> getComponents(std::string_view type);

    auto begin() {
        return this->components_.begin();
    }

    auto end() {
        return this->components_.end();
    }

private:
    std::map<std::string, std::unique_ptr<Component>, std::less<>> components_;
    std::map<std::string, std::map<std::string, Component*, std::less<>>, std::less<>>
        componentsByType_;
    std::vector<Component*> pendingRemoval_;
};

} // namespace sge::scripting
