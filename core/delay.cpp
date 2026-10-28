#include "delay.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace ukefx {

void Delay::prepare(int sampleRate, int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    // Round up to a power of two so wrapping is a bitwise AND instead of a modulo (a division).
    const auto needed = static_cast<std::size_t>(std::ceil(kMaxTimeSeconds * sampleRate)) + 1;
    buffer_.assign(std::bit_ceil(needed), 0.0f);
    mask_ = buffer_.size() - 1;
    write_ = 0;
}

void Delay::setFeedback(float feedback) {
    // At 1 or above every repeat is as loud as the last or louder, and the signal
    // grows without bound. Clamping here keeps that out of the audio thread.
    feedback_.store(std::clamp(feedback, 0.0f, 0.95f), std::memory_order_relaxed);
}

void Delay::setMix(float mix) {
    mix_.store(std::clamp(mix, 0.0f, 1.0f), std::memory_order_relaxed);
}

void Delay::process(const float* in, float* out, int n) {
    const float feedback = feedback_.load(std::memory_order_relaxed);
    const float mix = mix_.load(std::memory_order_relaxed);
    const float timeMs = timeMs_.load(std::memory_order_relaxed);

    // At least 1 sample: reading the slot we are about to write would return audio
    // from a full buffer ago. At most mask_ so we never read past the oldest sample.
    const auto delay = std::clamp<std::size_t>(
        static_cast<std::size_t>(std::lround(timeMs * 0.001f * static_cast<float>(sampleRate_))),
        1, mask_);

    for (int i = 0; i < n; ++i) {
        const float dry = in[i]; // Read before writing out[i]: in and out may alias.
        const float wet = buffer_[(write_ - delay) & mask_];
        buffer_[write_] = dry + feedback * wet;
        out[i] = (1.0f - mix) * dry + mix * wet;
        write_ = (write_ + 1) & mask_;
    }
}

} // namespace ukefx
