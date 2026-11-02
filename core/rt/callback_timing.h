#pragma once

#include <cstdint>

namespace minerva {

// One audio callback, as measured by the backend. Times are in the ticks of the
// backend's clock (QueryPerformanceCounter on Windows), so core stays free of OS
// calls; TimingStats converts them with the clock's ticks-per-second.
//
// Plain data with no heap memory: the audio thread copies one into a queue per
// callback, and that must never allocate.
struct CallbackTiming {
    std::int64_t startTicks = 0;    // when the callback began
    std::int64_t durationTicks = 0; // how long it worked
    std::uint32_t frames = 0;       // block size, which sets the deadline
};

} // namespace minerva
