#include "plot/timing_history.h"

#include <algorithm>

namespace gui {

TimingHistory::TimingHistory(double seconds, double callbacksPerSecond, double ticksPerSecond)
    : ticksPerSecond_(ticksPerSecond), capacity_(static_cast<std::size_t>(seconds * callbacksPerSecond) + 1) {
    times_.reserve(capacity_);
    durations_.reserve(capacity_);
}

void TimingHistory::add(const minerva::CallbackTiming& timing) {
    if (firstStart_ < 0) firstStart_ = timing.startTicks;
    const double time = static_cast<double>(timing.startTicks - firstStart_) / ticksPerSecond_;
    // The graph's axis is logarithmic, which has no place for zero.
    const double duration = std::max(0.1, 1e6 * static_cast<double>(timing.durationTicks) / ticksPerSecond_);
    latest_ = time;

    if (times_.size() < capacity_) {
        times_.push_back(time); // within the reserve: no allocation
        durations_.push_back(duration);
        return;
    }
    times_[next_] = time;
    durations_[next_] = duration;
    next_ = (next_ + 1) % static_cast<int>(capacity_);
}

} // namespace gui
