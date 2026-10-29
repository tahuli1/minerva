#pragma once

namespace minerva {

// Attack, decay and release are times in seconds; sustain is a level in [0, 1].
struct AdsrParams {
    float attack = 0.01f;  // time to rise from 0 to 1
    float decay = 0.1f;    // time to fall from 1 to the sustain level
    float sustain = 0.8f;  // level held while the key is down
    float release = 0.2f;  // time to fade to 0 after the key goes up
};

// ADSR envelope: a level in [0, 1] that changes over time and multiplies a waveform.
// Each stage is a straight line that lands exactly on its target after a known
// number of samples, like SmoothedValue.
class Envelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    // Not RT: call before audio starts.
    void setSampleRate(int sampleRate) { sampleRate_ = static_cast<float>(sampleRate); }
    void setParams(const AdsrParams& params) { params_ = params; }

    // Both start from the current level, never from 0, so retriggering a note that
    // is still fading out does not jump (no click).
    void noteOn();
    void noteOff();

    // RT: advance one sample and return the new level.
    float next();

    bool isActive() const { return stage_ != Stage::Idle; }
    Stage stage() const { return stage_; }
    float level() const { return level_; }

private:
    void enter(Stage stage);
    void rampTo(float target, float seconds);

    AdsrParams params_;
    float sampleRate_ = 48000.0f;
    Stage stage_ = Stage::Idle;
    float level_ = 0.0f;
    float target_ = 0.0f;
    float step_ = 0.0f;
    int remaining_ = 0; // samples left in the current ramp
};

} // namespace minerva
