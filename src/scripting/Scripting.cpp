#include "scripting/Scripting.hpp"

#include <lua/lua.hpp>

#include "scripting/Component.hpp"
#include "scripting/Libs.hpp"

namespace sge::scripting {

namespace {

lua_State* initializeGlobalState() {
    auto* state = luaL_newstate();
    luaL_openlibs(state);
    return state;
}

lua_State* GlobalLuaState;

} // namespace

void Initialize() {
    GlobalLuaState = initializeGlobalState();
    InitializeComponentTypes();
    InitializeScriptingLibs();
    InitializeScriptingClasses();
}

lua_State* GetGlobalState() {
    return GlobalLuaState;
}

} // namespace sge::scripting
