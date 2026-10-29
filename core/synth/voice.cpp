#include "synth/voice.h"

#include <cmath>

namespace minerva {

float Voice::noteToHz(int note) {
    // A4 (note 69) is 440 Hz; every semitone multiplies the frequency by 2^(1/12),
    // so 12 semitones (an octave) double it.
    return 440.0f * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
}

void Voice::prepare(int sampleRate) {
    oscillator_.setSampleRate(sampleRate);
    envelope_.setSampleRate(sampleRate);
}

void Voice::start(int note, float velocity) {
    // A silent voice starts its waveform from the beginning of a cycle. One that is
    // still sounding keeps its phase, so the waveform doesn't jump (no click).
    if (!isActive()) oscillator_.reset();
    note_ = note;
    velocity_ = velocity;
    oscillator_.setFrequency(noteToHz(note));
    envelope_.noteOn();
}

void Voice::release() { envelope_.noteOff(); }

void Voice::render(float* out, int n) {
    if (!isActive()) return;
    for (int i = 0; i < n; ++i) out[i] += velocity_ * oscillator_.next() * envelope_.next();
}

} // namespace minerva
