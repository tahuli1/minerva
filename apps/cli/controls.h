#pragma once

#include "engine/engine.h"
#include "note_keys.h"

namespace cli {

// The values the user controls from the keyboard. Owned by the main thread, which
// pushes them into the engine: effect parameters through their atomic setters, the
// waveform through the command queue. The audio thread never sees this struct.
struct Settings {
    int octave = NoteKeys::kDefaultOctave;
    float velocity = 0.8f; // leaves room to play louder
    minerva::Waveform waveform = minerva::Waveform::Triangle; // soft without a filter
    // Off by default: distorting a chord adds the sums and differences of its notes'
    // frequencies (intermodulation), which sound harsh; one note at a time is fine.
    bool distortion = false;
    float drive = 0.4f; // warm overdrive rather than fuzz (DaisySP's 0..1 scale)
    float feedback = 0.4f;
    float mix = 0.35f;
};

void apply(const Settings& settings, minerva::Engine& engine);

// `key` is a Windows virtual key code. Returns true if the key changed a setting.
// Note keys are not settings; NoteKeys handles them.
bool handleKey(int key, Settings& settings);

void printHelp();
void printSettings(const Settings& settings);

} // namespace cli
