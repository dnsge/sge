#pragma once

#include <lua/lua.hpp>
#include <rva/variant.hpp>
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/Map.h>
#include <LuaBridge/Vector.h>

#include "util/Visit.hpp"

#include <cassert>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace sge::scripting {

using LuaValue = rva::variant<std::monostate,                    // nil
                              double,                            // number
                              bool,                              // boolean
                              std::string,                       // string
                              std::vector<rva::self_t>,          // array
                              std::map<std::string, rva::self_t> // table
                              >;

namespace detail {

inline bool LuaTableIsProbablyArray(const luabridge::LuaRef &ref) {
    assert(ref.isTable());
    auto* l = ref.state();

    ref.push(l);                     // Push the table we want to inspect
    lua_rawgeti(l, -1, 1);           // Try to access field "1"
    bool exists = !lua_isnil(l, -1); // Check whether the field is nil
    lua_pop(l, 2);                   // Clean up stack

    return exists;
}

} // namespace detail

} // namespace sge::scripting

template <>
struct luabridge::Stack<sge::scripting::LuaValue> {
    static void push(lua_State* L, const sge::scripting::LuaValue &v) {
        std::visit(sge::util::Visitor{
                       // Specialization for nil
                       [L](const std::monostate &) {
                           luabridge::Stack<luabridge::Nil>::push(L, luabridge::Nil{});
                       },
                       // All other variant types
                       [L]<typename T>(const T &v) {
                           luabridge::Stack<T>::push(L, v);
                       },
                   },
                   v);
    }

    static sge::scripting::LuaValue get(lua_State* L, int index) {
        auto ref = luabridge::Stack<luabridge::LuaRef>::get(L, index);
        switch (ref.type()) {
        case LUA_TNIL:
            return {std::monostate{}};
        case LUA_TNUMBER:
            return {ref.cast<double>()};
        case LUA_TBOOLEAN:
            return {ref.cast<bool>()};
        case LUA_TSTRING:
            return {ref.cast<std::string>()};
        case LUA_TTABLE:
            {
                if (sge::scripting::detail::LuaTableIsProbablyArray(ref)) {
                    // Interpret as vector
                    return {ref.cast<std::vector<sge::scripting::LuaValue>>()};
                } else {
                    return {ref.cast<std::map<std::string, sge::scripting::LuaValue>>()};
                }
            }
        default:
            throw std::runtime_error("lua type does not support event replication");
        }
    }

    static bool isInstance(lua_State* L, int index) {
        switch (lua_type(L, index)) {
        case LUA_TNIL:
        case LUA_TNUMBER:
        case LUA_TBOOLEAN:
        case LUA_TSTRING:
        case LUA_TTABLE:
            return true;
        default:
            return false;
        }
    }
};
