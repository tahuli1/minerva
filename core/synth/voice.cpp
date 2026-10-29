#include "synth/voice.h"

#include <cmath>

namespace minerva {

namespace {

// DaisySP's decay and release are exponential curves, and the time you give them is
// a time constant: the level covers about two thirds of the way in that time and
// only reaches its target after several more. Its release aims slightly below zero
// (-0.01) and stops when it crosses zero, so from full level that takes
// ln((1 + 0.01) / 0.01) = ln(101) time constants. Dividing our times by ln(101) makes
// them mean what they say: after `release` seconds the voice is guaranteed silent,
// which also bounds how long a voice can stay busy in the pool.
// (Its attack is already designed to reach full level in exactly the given time.)
const float kTimeConstantsToSilence = std::log(101.0f);

} // namespace

float Voice::noteToHz(int note) {
    // A4 (note 69) is 440 Hz; every semitone multiplies the frequency by 2^(1/12),
    // so 12 semitones (an octave) double it.
    return 440.0f * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
}

void Voice::prepare(int sampleRate) {
    oscillator_.Init(static_cast<float>(sampleRate));
    oscillator_.SetAmp(1.0f); // DaisySP's default is 0.5; loudness comes from velocity
    applyWaveform();
    envelope_.Init(static_cast<float>(sampleRate));
    applyEnvelope();
}

void Voice::setWaveform(Waveform waveform) {
    waveform_ = waveform;
    applyWaveform();
}

void Voice::applyWaveform() {
    // The PolyBLEP variants round off the sudden jumps of saw and square (and the
    // corners of triangle), which would otherwise alias into harsh tones at high notes.
    using O = daisysp::Oscillator;
    int shape = O::WAVE_SIN;
    switch (waveform_) {
    case Waveform::Sine: shape = O::WAVE_SIN; break;
    case Waveform::Triangle: shape = O::WAVE_POLYBLEP_TRI; break;
    case Waveform::Saw: shape = O::WAVE_POLYBLEP_SAW; break;
    case Waveform::Square: shape = O::WAVE_POLYBLEP_SQUARE; break;
    }
    oscillator_.SetWaveform(static_cast<uint8_t>(shape)); // DaisySP takes the id as a byte
}

void Voice::setEnvelope(const AdsrParams& params) {
    envelopeParams_ = params;
    applyEnvelope();
}

void Voice::applyEnvelope() {
    const AdsrParams& p = envelopeParams_;
    envelope_.SetTime(daisysp::ADSR_SEG_ATTACK, p.attack);
    envelope_.SetTime(daisysp::ADSR_SEG_DECAY, p.decay / kTimeConstantsToSilence);
    envelope_.SetTime(daisysp::ADSR_SEG_RELEASE, p.release / kTimeConstantsToSilence);
    envelope_.SetSustainLevel(p.sustain);
}

void Voice::start(int note, float velocity) {
    // A silent voice starts its waveform from the beginning of a cycle. One that is
    // still sounding keeps its phase, so the waveform doesn't jump (no click).
    if (!isActive()) oscillator_.Reset();
    // A voice taken over while its key is still held never sees the gate go down,
    // so restart the attack explicitly, from the current level (soft retrigger).
    if (gate_) envelope_.Retrigger(false);
    note_ = note;
    velocity_ = velocity;
    oscillator_.SetFreq(noteToHz(note));
    gate_ = true;
}

void Voice::release() { gate_ = false; }

void Voice::render(float* out, int n) {
    if (!isActive()) return;
    float level = level_;
    for (int i = 0; i < n; ++i) {
        level = envelope_.Process(gate_);
        out[i] += velocity_ * oscillator_.Process() * level;
    }
    level_ = level;
}

} // namespace minerva
