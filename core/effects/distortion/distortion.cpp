#include "effects/distortion/distortion.h"

#include <algorithm>

namespace minerva {

void Distortion::setDrive(float drive) {
    drive_.store(std::clamp(drive, kMinDrive, kMaxDrive), std::memory_order_relaxed);
}

void Distortion::prepare(int sampleRate, int /*maxBlockSize*/) {
    overdrive_.Init();
    appliedDrive_ = -1.0f;
    driveSmoothed_.reset(sampleRate, kParamRampSeconds, drive_.load(std::memory_order_relaxed));
    levelSmoothed_.reset(sampleRate, kParamRampSeconds, level_.load(std::memory_order_relaxed));
}

void Distortion::process(const float* in, float* out, int n) {
    driveSmoothed_.setTarget(drive_.load(std::memory_order_relaxed));
    levelSmoothed_.setTarget(level_.load(std::memory_order_relaxed));

    for (int i = 0; i < n; ++i) {
        // SetDrive recomputes DaisySP's gains: only call it while the drive is moving.
        const float drive = driveSmoothed_.next();
        if (drive != appliedDrive_) {
            overdrive_.SetDrive(drive);
            appliedDrive_ = drive;
        }
        out[i] = levelSmoothed_.next() * overdrive_.Process(in[i]);
    }
}

} // namespace minerva
