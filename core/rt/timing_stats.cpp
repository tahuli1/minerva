#include "rt/timing_stats.h"

#include <algorithm>
#include <cmath>

namespace minerva {

TimingStats::TimingStats(double ticksPerSecond, int sampleRate)
    : ticksPerSecond_(ticksPerSecond), sampleRate_(sampleRate) {}

void TimingStats::add(const CallbackTiming& timing) {
    if (lastStart_ >= 0) maxGap_ = std::max(maxGap_, timing.startTicks - lastStart_);
    lastStart_ = timing.startTicks;
    durations_.push_back(timing.durationTicks);
    totalDuration_ += timing.durationTicks;
    frames_ = timing.frames;
}

TimingSummary TimingStats::summary() const {
    TimingSummary s;
    s.callbacks = static_cast<int>(durations_.size());
    s.maxGapMs = ticksToMs(maxGap_);
    s.deadlineMs = sampleRate_ > 0 ? 1000.0 * frames_ / sampleRate_ : 0.0;
    if (durations_.empty()) return s;

    s.avgMs = ticksToMs(totalDuration_) / s.callbacks;
    std::vector<std::int64_t> sorted = durations_;
    std::sort(sorted.begin(), sorted.end());
    s.maxMs = ticksToMs(sorted.back());
    // The smallest duration that at least 99.9% of the callbacks stay at or below.
    const auto rank = static_cast<std::size_t>(std::ceil(0.999 * static_cast<double>(sorted.size())));
    s.p999Ms = ticksToMs(sorted[rank - 1]);
    return s;
}

void TimingStats::reset() {
    durations_.clear();
    totalDuration_ = 0;
    maxGap_ = 0;
}

double TimingStats::ticksToMs(std::int64_t ticks) const {
    return 1000.0 * static_cast<double>(ticks) / ticksPerSecond_;
}

} // namespace minerva
