#pragma once

#include "dsp/smoothed_value.h"
#include "effect.h"

#include <atomic>

namespace ukefx {

// Soft-clipping distortion: out = level * tanh(drive * in).
// tanh is ~linear for small inputs and saturates smoothly towards +/-1,
// which sounds warmer than hard clipping.
class Distortion : public Effect {
public:
    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    // Safe to call from any thread while audio runs. Changes glide over
    // kParamRampSeconds; prepare() starts from the values set so far, with no glide.
    void setDrive(float drive) { drive_.store(drive, std::memory_order_relaxed); }
    void setLevel(float level) { level_.store(level, std::memory_order_relaxed); }

private:
    // Atomics so the main thread can change them while the audio thread reads.
    // Relaxed is enough: each is an independent value, nothing is published through it.
    std::atomic<float> drive_{1.0f};
    std::atomic<float> level_{1.0f};

    // Audio-thread copies that glide towards the atomics' values.
    SmoothedValue driveSmoothed_;
    SmoothedValue levelSmoothed_;

    static_assert(std::atomic<float>::is_always_lock_free);
};

} // namespace ukefx
