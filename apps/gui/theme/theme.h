#pragma once

#include <imgui.h>

namespace gui {

// The look from the approved mockup: a dark panel, purple for controls and
// graphs, orange for what is happening now (a pressed key, the deadline), green
// for "all good" (audio on, no late callbacks).
namespace color {
inline constexpr ImU32 rgb(unsigned hex) {
    return IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 0xFF);
}
inline constexpr ImU32 kBackground = rgb(0x111317);
inline constexpr ImU32 kPanel = rgb(0x1B1E24);
inline constexpr ImU32 kInset = rgb(0x23272F);
inline constexpr ImU32 kLine = rgb(0x2E333C);
inline constexpr ImU32 kDim = rgb(0x4A505A);
inline constexpr ImU32 kText = rgb(0xE8EAED);
inline constexpr ImU32 kMuted = rgb(0xA3A9B3);
inline constexpr ImU32 kPurple = rgb(0xA78BFA);
inline constexpr ImU32 kPurpleLight = rgb(0xC4B5FD);
inline constexpr ImU32 kPurpleBack = rgb(0x221C33);
inline constexpr ImU32 kOrange = rgb(0xFF8A3D);
inline constexpr ImU32 kGreen = rgb(0x7FD49A);
inline constexpr ImU32 kKeyWhite = rgb(0xF2F0EB);
inline constexpr ImU32 kKeyBlack = rgb(0x1A1C21);
} // namespace color

// The fonts loaded by applyTheme(): Segoe UI and Consolas ship with Windows.
struct Fonts {
    ImFont* body = nullptr;     // labels and text
    ImFont* title = nullptr;    // MINERVA, section titles
    ImFont* number = nullptr;   // values, monospaced so digits don't jump
    ImFont* bigNumber = nullptr;
};

// Sets ImGui's colors, rounding and spacing, and loads the fonts. Call once,
// after ImGui::CreateContext() and before the first frame.
Fonts applyTheme();

} // namespace gui
