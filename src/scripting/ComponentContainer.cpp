#include "scripting/ComponentContainer.hpp"

#include "scripting/Component.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sge::scripting {

ComponentContainer::ComponentContainer(
    std::map<std::string, std::unique_ptr<Component>, std::less<>> &&components)
    : components_(std::move(components)) {
    for (auto &node : this->components_) {
        this->componentsByType_[node.second->type].emplace(node.second->key, node.second.get());
    }
}

ComponentContainer::~ComponentContainer() {
    while (!this->components_.empty()) {
        this->components_.erase(this->components_.begin());
    }
}

Component* ComponentContainer::addComponent(std::string_view key,
                                            std::unique_ptr<Component> &&component) {
    assert(!this->components_.contains(key));
    const auto &[node, _] = this->components_.emplace(key, std::move(component));
    this->componentsByType_[node->second->type].emplace(key, node->second.get());
    return node->second.get();
}

void ComponentContainer::removeComponent(std::string_view key) {
    auto it = this->components_.find(key);
    if (it == this->components_.end()) {
        return;
    }

    it->second->setEnabled(false);

    // Extract component from map
    auto node = this->components_.extract(it);

    // Remove component pointer from types mapping
    auto &typeMap = this->componentsByType_[node.mapped()->type];
    auto eraseIt = std::find_if(typeMap.begin(), typeMap.end(), [&](const auto &typeNode) {
        return typeNode.second == node.mapped().get();
    });
    typeMap.erase(eraseIt);
}

void ComponentContainer::removeComponent(Component* component) {
    // Find key of component to remove
    auto it =
        std::find_if(this->components_.begin(), this->components_.end(), [&](const auto &node) {
            return node.second.get() == component;
        });
    if (it == this->components_.end()) {
        return;
    }
    this->removeComponent(it->first);
}

void ComponentContainer::removeComponentLater(Component* component) {
    this->pendingRemoval_.push_back(component);
}

void ComponentContainer::removeDeferred() {
    for (auto* toRemove : this->pendingRemoval_) {
        this->removeComponent(toRemove);
    }
    this->pendingRemoval_.clear();
}

Component* ComponentContainer::getComponentByKey(std::string_view key) {
    auto it = this->components_.find(key);
    if (it == this->components_.end()) {
        return nullptr;
    }
    return it->second.get();
}

Component* ComponentContainer::getComponent(std::string_view type) {
    auto it = this->componentsByType_.find(type);
    if (it == this->componentsByType_.end() || it->second.empty()) {
        return nullptr;
    }
    return it->second.begin()->second;
}

Component* ComponentContainer::getComponent(const luabridge::LuaRef &componentRef) {
    auto it =
        std::find_if(this->components_.begin(), this->components_.end(), [&](const auto &node) {
            return node.second->ref() == componentRef;
        });
    if (it == this->components_.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::vector<Component*> ComponentContainer::getComponents(std::string_view type) {
    std::vector<Component*> res;
    auto it = this->componentsByType_.find(type);
    if (it == this->componentsByType_.end()) {
        return res;
    }
    res.reserve(it->second.size());
    for (const auto &node : it->second) {
        res.push_back(node.second);
    }
    return res;
}

} // namespace sge::scripting
