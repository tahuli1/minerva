#pragma once

namespace minerva {

enum class Waveform { Sine, Triangle, Saw, Square };

// Generates a repeating waveform. It keeps a phase that goes from 0 to 1 once per
// cycle (the "clock hand") and turns it into a sample in [-1, 1].
//
// The phase wraps back to 0 after every cycle, so it never grows. Computing
// sin(2*pi*f*t) with an ever-growing t would lose float precision after a few
// minutes and the pitch would drift.
class Oscillator {
public:
    // Not RT: call before audio starts.
    void setSampleRate(int sampleRate);

    void setFrequency(float hz);
    void setWaveform(Waveform waveform) { waveform_ = waveform; }
    void reset() { phase_ = 0.0f; }

    // Off only to compare against the naive waveform (tests, the scale tool).
    void setAntiAliasing(bool on) { antiAliasing_ = on; }

    // RT: the next sample.
    float next();

private:
    float sampleRate_ = 48000.0f;
    float frequency_ = 440.0f;
    float phase_ = 0.0f;     // where we are in the cycle, 0..1
    float increment_ = 0.0f; // how far the phase moves per sample: frequency / sampleRate
    Waveform waveform_ = Waveform::Sine;
    bool antiAliasing_ = true;
};

} // namespace minerva
