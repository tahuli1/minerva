#pragma once

#include <imgui.h>

namespace gui {

// Controls ImGui doesn't have, drawn with its draw list in the mockup's style.
// Each is one call per frame (immediate mode) and returns true when the user
// changed something.

// A rotary knob: drag up or down to turn it. Dimmed when `enabled` is false.
bool knob(const char* id, const char* label, float* value, float min, float max, bool enabled);

enum class WaveShape { Sine, Triangle, Saw, Square };
// A big pad with a picture of the waveform; purple when selected.
bool waveformPad(const char* id, const char* label, WaveShape shape, bool selected, ImVec2 size);

// A rounded on/off switch with a text label.
bool pillToggle(const char* id, const char* label, bool on);

// A small card: a muted title over a large value.
void statCard(const char* title, const char* value, ImU32 valueColor, ImVec2 size, ImFont* valueFont);

// A muted section title, in the spaced capitals of the mockup.
void sectionTitle(const char* text);

} // namespace gui
