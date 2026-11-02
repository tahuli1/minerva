#include "rt/timing_stats.h"

#include <algorithm>
#include <cmath>

namespace minerva {

TimingStats::TimingStats(double ticksPerSecond, int sampleRate)
    : ticksPerSecond_(ticksPerSecond), sampleRate_(sampleRate) {}

void TimingStats::add(const CallbackTiming& timing) {
    if (lastStart_ >= 0) {
        const std::int64_t gap = timing.startTicks - lastStart_;
        maxGap_ = std::max(maxGap_, gap);
        // The block period in clock ticks follows the block size, so the threshold
        // stays right if the buffer size changes.
        const double periodTicks = ticksPerSecond_ * timing.frames / sampleRate_;
        if (static_cast<double>(gap) > kLateFactor * periodTicks) ++lateCallbacks_;
    }
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
    s.lateCallbacks = lateCallbacks_;
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
    lateCallbacks_ = 0;
}

double TimingStats::ticksToMs(std::int64_t ticks) const {
    return 1000.0 * static_cast<double>(ticks) / ticksPerSecond_;
}

} // namespace minerva
