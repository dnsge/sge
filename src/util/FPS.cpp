#include "util/FPS.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>

namespace sge::util {

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::time_point;

size_t FrameCounter = 0;
std::array<long long, 50> FrameTimesUs;
time_point<high_resolution_clock> FrameStartTime;

void StartFrame() {
    FrameStartTime = high_resolution_clock::now();
}

void EndFrame() {
    auto end = high_resolution_clock::now();
    auto usElapsed = duration_cast<microseconds>(end - FrameStartTime).count();
    FrameTimesUs[FrameCounter] = usElapsed;
    FrameCounter = (FrameCounter + 1) % FrameTimesUs.size();

    long long total = 0;
    for (auto time : FrameTimesUs) {
        total += time;
    }
    double avg = (double)total / FrameTimesUs.size();
    double fps = 1000000.0 / avg;
    if (usElapsed > 2000) {
        std::cout << "Last frame time: " << usElapsed / 1000 << " ms" << std::endl;
    } else {
        std::cout << "Last frame time: " << usElapsed << " µs" << std::endl;
    }
    std::cout << "Average FPS: " << std::setprecision(3) << fps << std::endl;
}

} // namespace sge::util
