#pragma once

#include "rt/callback_timing.h"

#include <cstdint>
#include <vector>

namespace gui {

// The last few seconds of callback durations, for the live graph: a ring buffer
// sized once, at start. Not RT (main thread), but bounded all the same: an hour
// of playing uses the same memory as a minute, and every frame draws the same
// number of points.
//
// The arrays are in ring order: the oldest point is at offset(), and a plot
// starts there and wraps around (ImPlot takes the offset for exactly this).
class TimingHistory {
public:
    TimingHistory(double seconds, double callbacksPerSecond, double ticksPerSecond);

    void add(const minerva::CallbackTiming& timing);

    int size() const { return static_cast<int>(times_.size()); }
    int offset() const { return next_; }
    const double* times() const { return times_.data(); }         // seconds since the first callback
    const double* durations() const { return durations_.data(); } // microseconds
    double latestTime() const { return latest_; }

private:
    double ticksPerSecond_;
    std::size_t capacity_;
    std::vector<double> times_;
    std::vector<double> durations_;
    int next_ = 0; // where the next point goes once the buffer is full
    std::int64_t firstStart_ = -1; // -1: no callback seen yet
    double latest_ = 0.0;
};

} // namespace gui
