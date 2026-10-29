#include "dsp/envelope.h"

#include <cmath>

namespace minerva {

void Envelope::noteOn() { enter(Stage::Attack); }

void Envelope::noteOff() {
    if (stage_ != Stage::Idle) enter(Stage::Release);
}

void Envelope::rampTo(float target, float seconds) {
    target_ = target;
    remaining_ = static_cast<int>(std::lround(seconds * sampleRate_));
    step_ = remaining_ > 0 ? (target - level_) / static_cast<float>(remaining_) : 0.0f;
}

void Envelope::enter(Stage stage) {
    stage_ = stage;
    switch (stage) {
    case Stage::Attack:
        // Same slope as a full 0 -> 1 attack, so from a higher level it arrives sooner.
        rampTo(1.0f, params_.attack * (1.0f - level_));
        break;
    case Stage::Decay:
        rampTo(params_.sustain, params_.decay);
        break;
    case Stage::Sustain:
        level_ = params_.sustain;
        remaining_ = 0;
        return;
    case Stage::Release:
        rampTo(0.0f, params_.release); // R is the fade time from wherever we are
        break;
    case Stage::Idle:
        level_ = 0.0f;
        remaining_ = 0;
        return;
    }
    // A zero-length stage finishes at once and moves on (e.g. attack 0 -> straight to decay).
    if (remaining_ == 0) {
        level_ = target_;
        enter(stage == Stage::Attack ? Stage::Decay : stage == Stage::Decay ? Stage::Sustain : Stage::Idle);
    }
}

float Envelope::next() {
    if (stage_ == Stage::Idle || stage_ == Stage::Sustain) return level_;

    // Land exactly on the target at the end instead of accumulating rounding error.
    level_ = (--remaining_ == 0) ? target_ : level_ + step_;
    if (remaining_ == 0)
        enter(stage_ == Stage::Attack ? Stage::Decay : stage_ == Stage::Decay ? Stage::Sustain : Stage::Idle);
    return level_;
}

} // namespace minerva
