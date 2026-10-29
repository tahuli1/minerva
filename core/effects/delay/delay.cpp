#include "effects/delay/delay.h"

#include <algorithm>
#include <cmath>

namespace minerva {

void Delay::prepare(int sampleRate, int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    line_ = std::make_unique<daisysp::DelayLine<float, kMaxDelaySamples>>();
    line_->Init();
    feedbackSmoothed_.reset(sampleRate, kParamRampSeconds, feedback_.load(std::memory_order_relaxed));
    mixSmoothed_.reset(sampleRate, kParamRampSeconds, mix_.load(std::memory_order_relaxed));
}

void Delay::setFeedback(float feedback) {
    // At 1 or above every repeat is at least as loud as the last: the signal grows forever.
    feedback_.store(std::clamp(feedback, 0.0f, 0.95f), std::memory_order_relaxed);
}

void Delay::setMix(float mix) {
    mix_.store(std::clamp(mix, 0.0f, 1.0f), std::memory_order_relaxed);
}

void Delay::process(const float* in, float* out, int n) {
    feedbackSmoothed_.setTarget(feedback_.load(std::memory_order_relaxed));
    mixSmoothed_.setTarget(mix_.load(std::memory_order_relaxed));

    // At least 1 sample: 0 would read the slot about to be written (the oldest audio).
    const float samples = timeMs_.load(std::memory_order_relaxed) * 0.001f * static_cast<float>(sampleRate_);
    line_->SetDelay(std::clamp<std::size_t>(static_cast<std::size_t>(std::lround(samples)), 1,
                                            kMaxDelaySamples - 1));

    for (int i = 0; i < n; ++i) {
        const float feedback = feedbackSmoothed_.next();
        const float mix = mixSmoothed_.next();
        const float dry = in[i]; // read before writing out[i]: in and out may alias
        const float wet = line_->Read();
        line_->Write(dry + feedback * wet);
        out[i] = (1.0f - mix) * dry + mix * wet;
    }
}

} // namespace minerva
