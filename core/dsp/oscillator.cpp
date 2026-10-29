#include "dsp/oscillator.h"

#include <algorithm>
#include <cmath>

namespace minerva {

namespace {

constexpr float kTwoPi = 6.28318530718f;

// PolyBLEP: a small polynomial correction applied only to the two samples around a
// sudden jump in the waveform (saw, square). A sharp jump contains harmonics above
// half the sample rate, which fold back as harsh, unrelated tones (aliasing);
// rounding the jump off removes most of them for almost no CPU.
// `t` is the phase relative to the jump, `dt` the phase increment per sample.
float polyBlep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

} // namespace

void Oscillator::setSampleRate(int sampleRate) {
    sampleRate_ = static_cast<float>(sampleRate);
    setFrequency(frequency_);
}

void Oscillator::setFrequency(float hz) {
    // Keep well below half the sample rate: above it a waveform cannot be represented.
    frequency_ = std::clamp(hz, 0.0f, 0.45f * sampleRate_);
    increment_ = frequency_ / sampleRate_;
}

float Oscillator::next() {
    const float t = phase_;
    float out = 0.0f;
    switch (waveform_) {
    case Waveform::Sine:
        out = std::sin(kTwoPi * t);
        break;
    case Waveform::Triangle:
        // No jumps, only corners, so aliasing is mild; left naive on purpose.
        out = t < 0.25f ? 4.0f * t : t < 0.75f ? 2.0f - 4.0f * t : 4.0f * t - 4.0f;
        break;
    case Waveform::Saw:
        out = 2.0f * t - 1.0f; // rises from -1 to 1, then jumps back at t = 0
        if (antiAliasing_) out -= polyBlep(t, increment_);
        break;
    case Waveform::Square:
        out = t < 0.5f ? 1.0f : -1.0f; // jumps at t = 0 and t = 0.5
        if (antiAliasing_) {
            out += polyBlep(t, increment_);
            out -= polyBlep(std::fmod(t + 0.5f, 1.0f), increment_);
        }
        break;
    }

    phase_ += increment_;
    if (phase_ >= 1.0f) phase_ -= 1.0f; // one full turn of the clock hand
    return out;
}

} // namespace minerva
