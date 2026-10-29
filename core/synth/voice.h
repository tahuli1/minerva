#pragma once

#include "dsp/envelope.h"
#include "dsp/oscillator.h"

namespace minerva {

// One playing note: an oscillator shaped by an envelope, plus what it is playing.
// Holds no heap memory, so a fixed pool of voices can be created before audio starts
// and reused forever.
class Voice {
public:
    // Not RT: call before audio starts.
    void prepare(int sampleRate);
    void setWaveform(Waveform waveform) { oscillator_.setWaveform(waveform); }
    void setEnvelope(const AdsrParams& params) { envelope_.setParams(params); }
    // Off only to hear the difference PolyBLEP makes (the scale tool).
    void setAntiAliasing(bool on) { oscillator_.setAntiAliasing(on); }

    // RT. `note` is a MIDI-style note number (60 = middle C, 69 = A 440 Hz);
    // `velocity` is the loudness, 0..1.
    void start(int note, float velocity);
    void release();

    // RT. Adds this voice's sound to `out` rather than overwriting it, so several
    // voices can be summed into one buffer (a chord is the sum of its notes).
    void render(float* out, int n);

    bool isActive() const { return envelope_.isActive(); }
    int note() const { return note_; }

    static float noteToHz(int note);

private:
    Oscillator oscillator_;
    Envelope envelope_;
    int note_ = -1;
    float velocity_ = 1.0f;
};

} // namespace minerva
