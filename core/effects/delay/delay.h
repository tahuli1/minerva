#pragma once

#include "Utility/delayline.h"
#include "dsp/smoothed_value.h"
#include "effects/effect.h"

#include <atomic>
#include <cstddef>
#include <memory>

namespace minerva {

// Feedback delay (echo) around DaisySP's DelayLine: each repeat is the previous one
// scaled by `feedback`, and `mix` blends dry and wet.
class Delay : public Effect {
public:
    static constexpr std::size_t kMaxDelaySamples = 96000; // 2 s at 48 kHz, ~384 KB

    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    // Safe from any thread. Feedback and mix glide over kParamRampSeconds; the time
    // does not, so changing it live clicks.
    void setTimeMs(float ms) { timeMs_.store(ms, std::memory_order_relaxed); }
    void setFeedback(float feedback);
    void setMix(float mix);

private:
    // DaisySP keeps the buffer inside the object: too big for a stack, so it is
    // allocated in prepare(), before audio starts.
    std::unique_ptr<daisysp::DelayLine<float, kMaxDelaySamples>> line_;
    int sampleRate_ = 48000;

    SmoothedValue feedbackSmoothed_;
    SmoothedValue mixSmoothed_;
    std::atomic<float> timeMs_{350.0f};
    std::atomic<float> feedback_{0.4f};
    std::atomic<float> mix_{0.35f};
};

} // namespace minerva
