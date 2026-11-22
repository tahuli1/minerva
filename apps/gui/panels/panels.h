#pragma once

#include "keyboard/controls.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"
#include "theme/theme.h"
#include "window/texture.h"

#include <imgui.h>

#include <span>

namespace gui {

// The window's sections, as in the mockup. Each draws itself for one frame;
// the ones with controls edit `settings` and say what changed, and the caller
// pushes the change to the engine the same way a key press does.
struct Changed {
    bool settings = false; // apply() the settings
    bool waveform = false; // and send the waveform command
};

// The logo's size on screen, in pixels; the logo is loaded at this size.
constexpr int kLogoSize = 56;

// Logo, name, and the stream's status on the right.
void drawHeader(const Fonts& fonts, const Texture& logo, const minerva::StreamInfo& info, int threadPriority);

Changed drawSoundPanel(app::Settings& settings, ImVec2 size);
Changed drawEffectsPanel(app::Settings& settings, ImVec2 size);
// The last second's callback timing.
void drawEnginePanel(const Fonts& fonts, const minerva::TimingSummary& timing, ImVec2 size);

// The current octave's C up to the next octave's F#: exactly the range the
// computer keys play.
// `held` counts, by note, what holds each note down. Returns the note the mouse
// holds down, or -1.
int drawKeysPanel(const app::Settings& settings, std::span<const int> held, ImVec2 size);

} // namespace gui
