#include "synth/synth.h"

namespace minerva {

namespace {

// Each voice is scaled down so that several full-volume notes together stay inside
// [-1, 1]: four voices at full velocity reach 1.
constexpr float kVoiceGain = 0.25f;

} // namespace

void Synth::prepare(int sampleRate) {
    for (Voice& v : voices_) v.prepare(sampleRate);
}

void Synth::setWaveform(Waveform waveform) {
    for (Voice& v : voices_) v.setWaveform(waveform);
}

void Synth::setEnvelope(const AdsrParams& params) {
    for (Voice& v : voices_) v.setEnvelope(params);
}

int Synth::pickVoice(int note) const {
    // 1. The same note is already sounding: play it again on that voice, rather than
    //    having one note on two voices.
    for (int i = 0; i < kVoices; ++i)
        if (voices_[i].isActive() && voices_[i].note() == note) return i;
    // 2. A free voice.
    for (int i = 0; i < kVoices; ++i)
        if (!voices_[i].isActive()) return i;
    // 3. Steal the quietest voice whose key is already released: the least audible loss.
    int best = -1;
    for (int i = 0; i < kVoices; ++i)
        if (!voices_[i].isHeld() && (best < 0 || voices_[i].level() < voices_[best].level())) best = i;
    if (best >= 0) return best;
    // 4. Every key is held: steal the oldest note.
    best = 0;
    for (int i = 1; i < kVoices; ++i)
        if (startedAt_[i] < startedAt_[best]) best = i;
    return best;
}

void Synth::noteOn(int note, float velocity) {
    const int i = pickVoice(note);
    voices_[i].start(note, velocity * kVoiceGain);
    startedAt_[i] = ++notesStarted_;
}

void Synth::noteOff(int note) {
    for (Voice& v : voices_)
        if (v.isHeld() && v.note() == note) v.release();
}

void Synth::render(float* out, int n) {
    for (Voice& v : voices_) v.render(out, n);
}

int Synth::activeVoices() const {
    int count = 0;
    for (const Voice& v : voices_)
        if (v.isActive()) ++count;
    return count;
}

bool Synth::isSounding(int note) const {
    for (const Voice& v : voices_)
        if (v.isActive() && v.note() == note) return true;
    return false;
}

} // namespace minerva
