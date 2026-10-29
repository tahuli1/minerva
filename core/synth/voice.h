#pragma once

#include "Control/adsr.h"
#include "Synthesis/oscillator.h"

namespace minerva {

enum class Waveform { Sine, Triangle, Saw, Square };

// Attack, decay and release are times in seconds; sustain is a level in [0, 1].
// Meaning, as in the lessons: attack is the time to reach full level, decay the time
// to (practically) reach the sustain level, release the longest time until silence.
struct AdsrParams {
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.8f;
    float release = 0.2f;
};

// One playing note: an oscillator shaped by an ADSR envelope (both from DaisySP),
// plus what it is playing. Holds no heap memory, so a fixed pool of voices can be
// created before audio starts and reused forever.
class Voice {
public:
    // Not RT: call before audio starts.
    void prepare(int sampleRate);
    void setWaveform(Waveform waveform);
    void setEnvelope(const AdsrParams& params);
    // Off only to hear the difference anti-aliasing (PolyBLEP) makes (the scale tool).
    void setAntiAliasing(bool on);

    // RT. `note` is a MIDI-style note number (60 = middle C, 69 = A 440 Hz);
    // `velocity` is the loudness, 0..1.
    void start(int note, float velocity);
    void release();

    // RT. Adds this voice's sound to `out` rather than overwriting it, so several
    // voices can be summed into one buffer (a chord is the sum of its notes).
    void render(float* out, int n);

    bool isActive() const { return gate_ || envelope_.IsRunning(); }
    int note() const { return note_; }

    static float noteToHz(int note);

private:
    // DaisySP's Init() resets waveform and envelope times, and the times depend on the
    // sample rate, so both are kept here and re-applied in prepare().
    void applyWaveform();
    void applyEnvelope();

    daisysp::Oscillator oscillator_;
    daisysp::Adsr envelope_;
    AdsrParams envelopeParams_;
    Waveform waveform_ = Waveform::Sine;
    bool antiAliasing_ = true;
    bool gate_ = false; // true while the key is held
    int note_ = -1;
    float velocity_ = 1.0f;
};

} // namespace minerva
