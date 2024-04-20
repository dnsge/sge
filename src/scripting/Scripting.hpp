#pragma once

#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

namespace sge::scripting {

void Initialize();
lua_State* GetGlobalState();

} // namespace sge::scripting
