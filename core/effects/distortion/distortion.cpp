#include "effects/distortion/distortion.h"

#include <cmath>

namespace ukefx {

void Distortion::prepare(int sampleRate, int /*maxBlockSize*/) {
    driveSmoothed_.reset(sampleRate, kParamRampSeconds, drive_.load(std::memory_order_relaxed));
    levelSmoothed_.reset(sampleRate, kParamRampSeconds, level_.load(std::memory_order_relaxed));
}

void Distortion::process(const float* in, float* out, int n) {
    // Pick up the latest values once per block; the smoothers spread any change over
    // the next kParamRampSeconds so the waveform never jumps (no zipper noise).
    driveSmoothed_.setTarget(drive_.load(std::memory_order_relaxed));
    levelSmoothed_.setTarget(level_.load(std::memory_order_relaxed));

    for (int i = 0; i < n; ++i) {
        const float drive = driveSmoothed_.next();
        const float level = levelSmoothed_.next();
        out[i] = level * std::tanh(drive * in[i]);
    }
}

} // namespace ukefx
