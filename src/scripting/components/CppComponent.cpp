#include "scripting/components/CppComponent.hpp"

#include "Realm.hpp"
#include "scripting/Component.hpp"

#include <string>
#include <utility>

namespace sge::scripting {

CppComponent::CppComponent(std::string type, Realm realm)
    : Component(std::move(type), realm) {}

bool CppComponent::getEnabled() const {
    return this->enabled;
}

void CppComponent::setEnabled(bool enabled) {
    this->enabled = enabled;
}

} // namespace sge::scripting
