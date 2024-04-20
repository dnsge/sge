#pragma once

#include <string>
#include <string_view>

namespace sge::scripting {

void BeginTrace(const char* lifecycle, const std::string &name);
void BeginTrace(const char* lifecycle, std::string_view name);
void AddEvent(const char* text);
void EndTrace();

} // namespace sge::scripting

#ifdef TRACING
#    define TRACE_BEGIN(lifecycle, name) sge::scripting::BeginTrace(lifecycle, name)
#    define TRACE_EVENT(name) sge::scripting::AddEvent(name)
#    define TRACE_END sge::scripting::EndTrace()
#else
#    define TRACE_BEGIN(lifecycle, name) ((void)0)
#    define TRACE_EVENT(name) ((void)0)
#    define TRACE_END ((void)0)
#endif
