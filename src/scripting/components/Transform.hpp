#pragma once

#include "Realm.hpp"
#include "net/Replicator.hpp"
#include "resources/Deserialize.hpp"
#include "scripting/Component.hpp"
#include "scripting/components/CppComponent.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sge::scripting {

class Transform : public CppComponent {
public:
    Transform(Realm realm);
    ~Transform() override = default;

    const luabridge::LuaRef &ref() const override;

    std::unique_ptr<Component> clone() const override;
    void setValues(const std::vector<std::pair<std::string, ComponentValueType>> &values) override;

    void replicatePush(net::ReplicatePush &r) override;
    void replicatePull(net::ReplicatePull &r) override;

    OpaqueComponentPointer __opaquePointer;

    float x{0.0F};
    float y{0.0F};
    float rotation{0.0F};

private:
    luabridge::LuaRef ref_;
};

} // namespace sge::scripting
