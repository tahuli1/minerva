#include "effects/distortion/distortion.h"

#include <algorithm>

namespace minerva {

void Distortion::setDrive(float drive) {
    drive_.store(std::clamp(drive, kMinDrive, kMaxDrive), std::memory_order_relaxed);
}

float Distortion::wetTarget() const {
    return enabled_.load(std::memory_order_relaxed) ? 1.0f : 0.0f;
}

void Distortion::prepare(int sampleRate, int /*maxBlockSize*/) {
    overdrive_.Init();
    appliedDrive_ = -1.0f;
    driveSmoothed_.reset(sampleRate, kParamRampSeconds, drive_.load(std::memory_order_relaxed));
    levelSmoothed_.reset(sampleRate, kParamRampSeconds, level_.load(std::memory_order_relaxed));
    wetSmoothed_.reset(sampleRate, kParamRampSeconds, wetTarget());
}

void Distortion::process(const float* in, float* out, int n) {
    driveSmoothed_.setTarget(drive_.load(std::memory_order_relaxed));
    levelSmoothed_.setTarget(level_.load(std::memory_order_relaxed));
    wetSmoothed_.setTarget(wetTarget());

    for (int i = 0; i < n; ++i) {
        // SetDrive recomputes DaisySP's gains: only call it while the drive is moving.
        const float drive = driveSmoothed_.next();
        if (drive != appliedDrive_) {
            overdrive_.SetDrive(drive);
            appliedDrive_ = drive;
        }
        // The overdrive runs even when bypassed: the cost per block is then the same
        // on or off, so switching it on can never push a block past its deadline.
        const float dry = in[i];
        const float wet = levelSmoothed_.next() * overdrive_.Process(dry);
        const float mix = wetSmoothed_.next();
        out[i] = mix * wet + (1.0f - mix) * dry;
    }
}

} // namespace minerva
