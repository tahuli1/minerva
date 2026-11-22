#pragma once

#include <imgui.h>

#include <span>

namespace gui {

// Controls ImGui doesn't have, drawn with its draw list in the mockup's style.
// Each is one call per frame (immediate mode) and returns true when the user
// changed something.

// A rotary knob: circle the mouse around it, clockwise for more. Dimmed, and
// fixed, when `enabled` is false.
bool knob(const char* id, const char* label, float* value, float min, float max, bool enabled);

enum class WaveShape { Sine, Triangle, Saw, Square };
// A big pad with a picture of the waveform; purple when selected.
bool waveformPad(const char* id, const char* label, WaveShape shape, bool selected, ImVec2 size);

// A rounded on/off switch with a text label.
bool pillToggle(const char* id, const char* label, bool on);

// A piano keyboard from `firstNote` (a C) for `labels.size()` notes. A key is lit
// while held[note] > 0, and shows labels[i] (the computer key that plays it, or 0
// for none). Returns the note under the mouse while the left button holds it down
// on the keyboard, or -1.
int pianoKeys(const char* id, int firstNote, std::span<const int> held, std::span<const char> labels, ImVec2 size);

// A small card: a muted title over a large value.
void statCard(const char* title, const char* value, ImU32 valueColor, ImVec2 size, ImFont* valueFont);

// A muted section title, in the spaced capitals of the mockup.
void sectionTitle(const char* text);

} // namespace gui
