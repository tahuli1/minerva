#pragma once

#include "effects/delay/delay.h"
#include "effects/distortion/distortion.h"

namespace cli {

// The master effects, in Minerva's fixed order: distortion -> delay (reverb later).
struct Effects {
    minerva::Distortion distortion;
    minerva::Delay delay;
};

// The values the user controls from the keyboard. Owned by the main thread, which
// pushes them into the effects' atomic setters; the audio thread never sees this.
struct Settings {
    float drive = 4.0f; // warm overdrive rather than fuzz
    float feedback = 0.4f;
    float mix = 0.35f;
};

void apply(const Settings& settings, Effects& effects);

// `key` comes from _getch(). Function keys arrive as two codes, a 0 or 0xE0 prefix
// followed by the key's own code; for those `extended` is true and `key` is the
// second code. Returns true if the key changed a setting.
bool handleKey(int key, bool extended, Settings& settings);

void printHelp();
void printSettings(const Settings& settings);

} // namespace cli
