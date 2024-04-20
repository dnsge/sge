#pragma once

#include "scripting/LuaInterface.hpp"

#include <memory>

namespace sge::scripting {

void InitializeScriptingLibs();
void InitializeScriptingClasses();
void InitializeInterface(std::unique_ptr<LuaInterface> interface);

} // namespace sge::scripting
