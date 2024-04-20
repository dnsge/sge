#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "Realm.hpp"
#include "scripting/Component.hpp"

#include <string>

namespace sge::scripting {

template <typename T, typename V>
T MustGet(const V &variant) {
    if constexpr (std::is_same_v<T, float>) {
        if (std::holds_alternative<float>(variant)) {
            return std::get<float>(variant);
        } else if (std::holds_alternative<int>(variant)) {
            return static_cast<float>(std::get<int>(variant));
        }
        return float{};
    }

    if (!std::holds_alternative<T>(variant)) {
        return T{};
    }
    return std::get<T>(variant);
}

class CppComponent : public Component {
public:
    CppComponent(std::string type, Realm realm);
    ~CppComponent() override = default;

    bool getEnabled() const override;
    void setEnabled(bool enabled) override;

    bool enabled;
};

} // namespace sge::scripting
