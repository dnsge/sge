#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

namespace sge::scripting {

template <typename F>
bool ActorInvoke(std::string_view actorName, F f) {
    try {
        f();
        return true;
    } catch (const luabridge::LuaException &e) {
        auto errorMessage = std::string{e.what()};
        // Consistent across platforms
        std::replace(errorMessage.begin(), errorMessage.end(), '\\', '/');
        std::cout << "\033[31m" << actorName << " : " << errorMessage << "\033[0m" << std::endl;
        return false;
    }
}

} // namespace sge::scripting
