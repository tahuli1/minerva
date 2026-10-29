#pragma once

#include "synth/voice.h"

#include <array>
#include <cstdint>

namespace minerva {

// A fixed pool of voices. A note takes a free voice, or steals one when all are busy.
// The pool never grows or shrinks, so playing notes never allocates.
class Synth {
public:
    // Enough for a few held chords plus the notes still fading after their keys are
    // released, and a known upper bound on the work per audio block.
    static constexpr int kVoices = 16;

    // Not RT: call before audio starts.
    void prepare(int sampleRate);
    void setWaveform(Waveform waveform);
    void setEnvelope(const AdsrParams& params);

    // RT. `velocity` is the loudness of the note, 0..1.
    void noteOn(int note, float velocity);
    void noteOff(int note);
    // RT. Adds every active voice into `out`.
    void render(float* out, int n);

    int activeVoices() const;
    bool isSounding(int note) const;

private:
    int pickVoice(int note) const;

    std::array<Voice, kVoices> voices_;
    // When each voice was last started, to find the oldest one to steal.
    std::array<std::uint64_t, kVoices> startedAt_{};
    std::uint64_t notesStarted_ = 0;
};

} // namespace minerva
