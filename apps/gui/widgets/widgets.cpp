#include "widgets/widgets.h"

#include "theme/theme.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gui {

namespace {

// Text centered horizontally on `x`, starting at height `y`.
void centeredText(ImDrawList* draw, float x, float y, ImU32 col, const char* text, ImFont* font = nullptr) {
    ImFont* f = font != nullptr ? font : ImGui::GetFont();
    const float size = f->FontSize;
    const ImVec2 extent = f->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    draw->AddText(f, size, ImVec2(x - extent.x / 2, y), col, text);
}

} // namespace

bool knob(const char* id, const char* label, float* value, float min, float max, bool enabled) {
    constexpr float kSize = 92.0f, kRadius = 34.0f;
    // The arc runs from bottom-left, over the top, to bottom-right: 270 degrees.
    constexpr float kStart = std::numbers::pi_v<float> * 0.75f;
    constexpr float kSpan = std::numbers::pi_v<float> * 1.5f;

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(kSize, kSize + 48));
    bool changed = false;
    if (ImGui::IsItemActive() && enabled) {
        // 200 pixels of mouse travel cover the whole range.
        const float delta = -ImGui::GetIO().MouseDelta.y * (max - min) / 200.0f;
        if (delta != 0.0f) {
            *value = std::clamp(*value + delta, min, max);
            changed = true;
        }
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 center(origin.x + kSize / 2, origin.y + kSize / 2);
    const float t = (*value - min) / (max - min);
    const float angle = kStart + kSpan * t;
    draw->AddCircleFilled(center, kRadius, color::kInset, 48);
    draw->PathArcTo(center, kRadius, kStart, kStart + kSpan, 48);
    draw->PathStroke(color::kLine, 0, 7.0f);
    draw->PathArcTo(center, kRadius, kStart, angle, 48);
    draw->PathStroke(enabled ? color::kPurple : color::kDim, 0, 7.0f);
    draw->AddLine(center, ImVec2(center.x + 24 * std::cos(angle), center.y + 24 * std::sin(angle)), color::kText, 3.0f);

    char text[16];
    std::snprintf(text, sizeof text, "%.2f", *value);
    centeredText(draw, center.x, origin.y + kSize + 2, color::kMuted, label);
    centeredText(draw, center.x, origin.y + kSize + 24, enabled ? color::kText : color::kMuted, text);
    return changed;
}

bool waveformPad(const char* id, const char* label, WaveShape shape, bool selected, ImVec2 size) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 q(p.x + size.x, p.y + size.y);
    draw->AddRectFilled(p, q, selected ? color::kPurpleBack : color::kInset, 12.0f);
    draw->AddRect(p, q, selected ? color::kPurple : (hovered ? color::kDim : color::kLine), 12.0f, 0, 2.0f);

    // The waveform, two cycles, in a 40x16 box above the label.
    const ImU32 ink = selected ? color::kPurple : color::kText;
    const float x0 = p.x + size.x / 2 - 20, y0 = p.y + size.y / 2 - 18, w = 40, h = 16;
    ImVec2 points[64];
    int n = 0;
    for (int i = 0; i <= 40; ++i) {
        const float x = static_cast<float>(i) / 40.0f; // 0..1 across two cycles
        const float phase = std::fmod(x * 2.0f, 1.0f);
        float y = 0;
        switch (shape) {
        case WaveShape::Sine: y = std::sin(phase * 2 * std::numbers::pi_v<float>); break;
        case WaveShape::Triangle: y = phase < 0.5f ? 4 * phase - 1 : 3 - 4 * phase; break;
        case WaveShape::Saw: y = 2 * phase - 1; break;
        case WaveShape::Square: y = phase < 0.5f ? 1.0f : -1.0f; break;
        }
        points[n++] = ImVec2(x0 + x * w, y0 + h / 2 - y * h / 2);
    }
    draw->AddPolyline(points, n, ink, 0, 2.2f);
    centeredText(draw, p.x + size.x / 2, p.y + size.y / 2 + 6, ink, label);
    return pressed;
}

bool pillToggle(const char* id, const char* label, bool on) {
    ImGui::PushID(id);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 size(textSize.x + 32, 36);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##pill", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 q(p.x + size.x, p.y + size.y);
    draw->AddRectFilled(p, q, on ? color::kPurpleBack : color::kInset, 18.0f);
    draw->AddRect(p, q, on ? color::kPurple : color::kLine, 18.0f, 0, 2.0f);
    draw->AddText(ImVec2(p.x + 16, p.y + (size.y - textSize.y) / 2), on ? color::kPurple : color::kMuted, label);
    ImGui::PopID();
    return pressed;
}

void statCard(const char* title, const char* value, ImU32 valueColor, ImVec2 size, ImFont* valueFont) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), color::kInset, 12.0f);
    draw->AddText(ImVec2(p.x + 14, p.y + 10), color::kMuted, title);
    draw->AddText(valueFont, valueFont->FontSize, ImVec2(p.x + 14, p.y + 34), valueColor, value);
}

void sectionTitle(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, color::kMuted);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

} // namespace gui
