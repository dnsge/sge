#include "scripting/Tracing.hpp"

#include "gea/Helper.h"

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace sge::scripting {

namespace {

std::string TraceName;
std::vector<const char*> TraceEvents;

} // namespace

void BeginTrace(const char* lifecycle, const std::string &name) {
    assert(TraceName.empty());
    TraceName = std::string{lifecycle} + " of " + name;
}

void BeginTrace(const char* lifecycle, std::string_view name) {
    BeginTrace(lifecycle, std::string{name});
}

void AddEvent(const char* text) {
    TraceEvents.push_back(text);
}

void EndTrace() {
    assert(!TraceName.empty());

    if (TraceEvents.empty()) {
        TraceName.clear();
        return;
    }

    auto frame = Helper::GetFrameNumber();
    std::cerr << "[TRACE Frame " << frame << "] " << TraceName << ": " << TraceEvents.size()
              << " events" << std::endl;
    for (const auto* event : TraceEvents) {
        std::cerr << event << ", ";
    }
    std::cerr << std::endl;

    TraceName.clear();
    TraceEvents.clear();
}

} // namespace sge::scripting
