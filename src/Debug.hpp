#pragma once

#include <sys/signal.h>

#include "gea/Helper.h"

#include <csignal>

namespace sge {

#ifdef DEBUG

inline void BreakpointOnFrame(int frame) {
    if (Helper::GetFrameNumber() == frame) {
#    if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
        __builtin_debugtrap();
#    else
        std::raise(SIGTRAP);
#    endif
    }
}

#else

#    define BreakpointOnFrame(frame) ((void)0)

#endif

} // namespace sge
