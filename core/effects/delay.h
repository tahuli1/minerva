#pragma once

#include "effect.h"

#include <atomic>
#include <cstddef>
#include <vector>

namespace ukefx {

// Feedback delay (echo) on a circular buffer.
// Each repeat is the previous one scaled by `feedback`; `mix` blends dry and wet.
class Delay : public Effect {
public:
    static constexpr float kMaxTimeSeconds = 2.0f;

    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    // Safe to call from any thread while audio runs.
    void setTimeMs(float ms) { timeMs_.store(ms, std::memory_order_relaxed); }
    void setFeedback(float feedback);
    void setMix(float mix);

private:
    std::vector<float> buffer_; // Sized in prepare(), never resized in process().
    std::size_t mask_ = 0;      // buffer_.size() - 1; the size is a power of two.
    std::size_t write_ = 0;
    int sampleRate_ = 48000;

    std::atomic<float> timeMs_{350.0f};
    std::atomic<float> feedback_{0.4f};
    std::atomic<float> mix_{0.35f};
};

} // namespace ukefx
