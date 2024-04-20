#include "scripting/components/Transform.hpp"

#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/Scripting.hpp"
#include "scripting/components/CppComponent.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::scripting {

Transform::Transform(Realm realm)
    : CppComponent("Transform", realm)
    , __opaquePointer{this}
    , ref_{GetGlobalState(), this} {}

const luabridge::LuaRef &Transform::ref() const {
    return this->ref_;
}

std::unique_ptr<Component> Transform::clone() const {
    auto newTransform = std::make_unique<Transform>(this->realm);
    newTransform->x = this->x;
    newTransform->y = this->y;
    newTransform->rotation = this->rotation;
    return newTransform;
}

void Transform::setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) {
    for (const auto &[name, val] : values) {
        if (name == "x") {
            this->x = MustGet<float>(val);
        } else if (name == "y") {
            this->y = MustGet<float>(val);
        } else if (name == "rotation") {
            this->rotation = MustGet<float>(val);
        }
    }
}

void Transform::replicatePush(net::ReplicatePush &r) {
    r.writeNumber(this->x);
    r.writeNumber(this->y);
    r.writeNumber(this->rotation);
}

void Transform::replicatePull(net::ReplicatePull &r) {
    this->x = r.readNumber();
    this->y = r.readNumber();
    this->rotation = r.readNumber();
}

} // namespace sge::scripting
