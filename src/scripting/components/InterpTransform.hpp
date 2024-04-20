#pragma once

#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/components/CppComponent.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::scripting {

class InterpTransform : public CppComponent {
public:
    InterpTransform(Realm realm);
    ~InterpTransform() override = default;

    const luabridge::LuaRef &ref() const override;

    std::unique_ptr<Component> clone() const override;
    void setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) override;

    void onUpdate(float dt) override;
    void replicatePush(net::ReplicatePush &r) override;
    void replicatePull(net::ReplicatePull &r) override;

    OpaqueComponentPointer __opaquePointer;

    float x{0.0F};
    float y{0.0F};
    float rotation{0.0F};

private:
    struct InterpState {
        float x;
        float y;
        float rotation;
        std::chrono::steady_clock::time_point time;

        InterpState() = default;

        InterpState(float x, float y, float rotation, std::chrono::steady_clock::time_point time)
            : x(x)
            , y(y)
            , rotation(rotation)
            , time(time) {}
    };

    luabridge::LuaRef ref_;

    std::deque<InterpState> interps_;
    InterpState interpStart_;
};

} // namespace sge::scripting
