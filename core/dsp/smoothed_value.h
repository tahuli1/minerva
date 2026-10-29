#pragma once

namespace minerva {

// Default time for a parameter to glide to a new value. Long enough to hide the
// step (no "zipper" clicks), short enough that a knob still feels immediate.
inline constexpr float kParamRampSeconds = 0.02f;

// Moves a parameter towards its target in a straight line over a fixed time.
// A linear ramp (rather than an exponential one) reaches the target exactly and
// in a known number of samples, which keeps it predictable and easy to test.
//
// Audio-thread only: setTarget() once per block with the latest value from the
// parameter's atomic, next() once per sample.
class SmoothedValue {
public:
    // Not RT: call from prepare(). Starts at `initial` with no ramp in progress.
    void reset(int sampleRate, float rampSeconds, float initial) {
        rampSamples_ = static_cast<int>(rampSeconds * static_cast<float>(sampleRate));
        current_ = target_ = initial;
        remaining_ = 0;
    }

    void setTarget(float target) {
        // Called every block with the same value most of the time: that must not
        // restart a ramp that is already heading there.
        if (target == target_) return;
        target_ = target;
        if (rampSamples_ <= 0) {
            current_ = target;
            remaining_ = 0;
            return;
        }
        // Start from wherever we are now, so a new target mid-ramp never jumps.
        remaining_ = rampSamples_;
        step_ = (target_ - current_) / static_cast<float>(rampSamples_);
    }

    float next() {
        if (remaining_ > 0) {
            // Land exactly on the target at the end instead of accumulating rounding error.
            current_ = (--remaining_ == 0) ? target_ : current_ + step_;
        }
        return current_;
    }

    float current() const { return current_; }

private:
    float current_ = 0.0f;
    float target_ = 0.0f;
    float step_ = 0.0f;
    int rampSamples_ = 0;
    int remaining_ = 0;
};

} // namespace minerva
