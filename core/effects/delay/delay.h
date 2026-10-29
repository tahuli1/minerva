#pragma once

#include "dsp/smoothed_value.h"
#include "effects/effect.h"

#include <atomic>
#include <cstddef>
#include <vector>

namespace minerva {

// Feedback delay (echo) on a circular buffer.
// Each repeat is the previous one scaled by `feedback`; `mix` blends dry and wet.
class Delay : public Effect {
public:
    static constexpr float kMaxTimeSeconds = 2.0f;

    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    // Safe to call from any thread while audio runs. Feedback and mix glide over
    // kParamRampSeconds; prepare() starts from the values set so far, with no glide.
    //
    // The time is NOT smoothed: changing it live jumps the read position and clicks.
    // Gliding it needs a fractional delay with interpolation (the read point moves
    // between samples), which is left for a modulated effect such as a chorus.
    void setTimeMs(float ms) { timeMs_.store(ms, std::memory_order_relaxed); }
    void setFeedback(float feedback);
    void setMix(float mix);

private:
    SmoothedValue feedbackSmoothed_;
    SmoothedValue mixSmoothed_;

    std::vector<float> buffer_; // Sized in prepare(), never resized in process().
    std::size_t mask_ = 0;      // buffer_.size() - 1; the size is a power of two.
    std::size_t write_ = 0;
    int sampleRate_ = 48000;

    std::atomic<float> timeMs_{350.0f};
    std::atomic<float> feedback_{0.4f};
    std::atomic<float> mix_{0.35f};
};

} // namespace minerva
