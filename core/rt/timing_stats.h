#pragma once

#include "rt/callback_timing.h"

#include <cstdint>
#include <vector>

namespace minerva {

// Summary of the callbacks seen since the last reset, in milliseconds.
struct TimingSummary {
    int callbacks = 0;
    double avgMs = 0.0;
    double maxMs = 0.0;
    // Only 1 in 1000 callbacks took longer. The average hides the rare slow block
    // that causes a click; the maximum and this percentile show it.
    double p999Ms = 0.0;
    // Longest time between the starts of two callbacks. Normally one block period;
    // much more means something outside our code (the OS, the driver) held the
    // audio thread back.
    double maxGapMs = 0.0;
    double deadlineMs = 0.0; // one block period: frames / sample rate
    int driverXruns = 0;
};

// Collects callback timings on the main thread and summarizes them. Not RT: it
// stores every duration in a vector to compute the percentile.
class TimingStats {
public:
    TimingStats(double ticksPerSecond, int sampleRate);

    void add(const CallbackTiming& timing);
    TimingSummary summary() const;
    // Starts a new period. The last start time is kept, so the first gap of the
    // next period is still measured.
    void reset();

private:
    double ticksToMs(std::int64_t ticks) const;

    double ticksPerSecond_;
    int sampleRate_;
    std::vector<std::int64_t> durations_;
    std::int64_t totalDuration_ = 0;
    std::int64_t maxGap_ = 0;
    std::int64_t lastStart_ = -1; // -1: no callback seen yet, so no gap to measure
    std::uint32_t frames_ = 0;
    int driverXruns_ = 0;
};

} // namespace minerva
