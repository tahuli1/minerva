#pragma once

#include "Effects/overdrive.h"
#include "dsp/smoothed_value.h"
#include "effects/effect.h"

#include <atomic>

namespace minerva {

// DaisySP's Overdrive: a soft clipper with normalized output, so more drive means more
// distortion, not more volume. `level` sets the final volume.
class Distortion : public Effect {
public:
    // At exactly 1.0 DaisySP's output jumps to ~3x full scale, so the drive stops short.
    static constexpr float kMinDrive = 0.05f;
    static constexpr float kMaxDrive = 0.95f;

    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    // Safe from any thread; changes glide over kParamRampSeconds.
    void setDrive(float drive);
    void setLevel(float level) { level_.store(level, std::memory_order_relaxed); }

private:
    daisysp::Overdrive overdrive_;
    float appliedDrive_ = -1.0f;

    std::atomic<float> drive_{0.4f};
    std::atomic<float> level_{1.0f};
    SmoothedValue driveSmoothed_;
    SmoothedValue levelSmoothed_;

    static_assert(std::atomic<float>::is_always_lock_free);
};

} // namespace minerva
