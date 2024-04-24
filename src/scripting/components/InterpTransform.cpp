#include "scripting/components/InterpTransform.hpp"

#include "Common.hpp"
#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/Scripting.hpp"
#include "scripting/components/CppComponent.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::scripting {

InterpTransform::InterpTransform(Realm realm)
    : CppComponent("InterpTransform", realm)
    , __opaquePointer{this}
    , ref_{GetGlobalState(), this} {}

const luabridge::LuaRef &InterpTransform::ref() const {
    return this->ref_;
}

std::unique_ptr<Component> InterpTransform::clone() const {
    auto newTransform = std::make_unique<InterpTransform>(this->realm);
    newTransform->x = this->x;
    newTransform->y = this->y;
    newTransform->rotation = this->rotation;
    return newTransform;
}

void InterpTransform::setValues(
    const std::vector<std::pair<std::string, ComponentValueType>> &values) {
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

void InterpTransform::onUpdate(float dt) {
    if (CurrentRealm() != GeneralRealm::Client) {
        return;
    }

    if (this->interps_.empty()) {
        if (this->actor->pendingServerDestroy()) {
            // We have no more interpolated movement so we can go ahead and actually
            // destroy this actor for good.
            this->actor->destroy();
        }
        return;
    }

    using namespace std::chrono;

    auto now = steady_clock::now();
    auto delta = duration_cast<microseconds>(now - this->interps_.front().time);
    float frac = static_cast<float>(delta.count()) / CurrentGame().tickDuration().count();

    while (frac >= 1.0F && this->interps_.size() >= 2) {
        this->interpStart_ = this->interps_.front();
        this->interps_.pop_front();
        delta = duration_cast<microseconds>(now - this->interps_.front().time);
        frac = static_cast<float>(delta.count()) / CurrentGame().tickDuration().count();
    }

    const auto &next = this->interps_.front();
    if (frac >= 1.0F) {
        this->x = next.x;
        this->y = next.y;
        this->rotation = next.rotation;
        this->interps_.pop_front();
    } else {
        this->x = (next.x - this->interpStart_.x) * frac + this->interpStart_.x;
        this->y = (next.y - this->interpStart_.y) * frac + this->interpStart_.y;
        this->rotation = this->interpStart_.rotation; // Don't interp rotation
    }
}

void InterpTransform::replicatePush(net::ReplicatePush &r) {
    r.writeNumber(this->x);
    r.writeNumber(this->y);
    r.writeNumber(this->rotation);
}

void InterpTransform::replicatePull(net::ReplicatePull &r) {
    if (!r.doInterp()) {
        // Standard non-interpolation behavior. Read the desired transform
        // and immediately update the component.
        this->x = r.readNumber();
        this->y = r.readNumber();
        this->rotation = r.readNumber();
        return;
    }

    auto now = std::chrono::steady_clock::now();

    if (this->interps_.empty()) {
        this->interpStart_ = InterpState{
            this->x,
            this->y,
            this->rotation,
            now,
        };
    }

    float ix = r.readNumber();
    float iy = r.readNumber();
    float ir = r.readNumber();

    this->interps_.emplace_back(ix, iy, ir, now);
}

} // namespace sge::scripting
