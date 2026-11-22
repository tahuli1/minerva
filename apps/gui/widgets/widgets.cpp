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

namespace {

bool isBlack(int note) {
    const int semitone = note % 12;
    return semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10;
}

// Where one key is drawn, and whether it is black.
struct KeyRect {
    ImVec2 min, max;
    bool black;
};

// A black key sits across the line between the white key before it and the next.
KeyRect keyRect(int whiteBefore, bool black, ImVec2 origin, ImVec2 size, float whiteWidth) {
    if (!black) {
        const float x = origin.x + whiteBefore * whiteWidth;
        return {ImVec2(x + 1, origin.y), ImVec2(x + whiteWidth - 1, origin.y + size.y), false};
    }
    const float width = whiteWidth * 0.6f;
    const float x = origin.x + whiteBefore * whiteWidth - width / 2;
    return {ImVec2(x, origin.y), ImVec2(x + width, origin.y + size.y * 0.62f), true};
}

} // namespace

int pianoKeys(const char* id, int firstNote, std::span<const int> held, std::span<const char> labels, ImVec2 size) {
    constexpr int kMaxKeys = 128; // every MIDI note
    const int count = std::min(static_cast<int>(labels.size()), kMaxKeys);
    int whiteCount = 0;
    for (int i = 0; i < count; ++i)
        if (!isBlack(firstNote + i)) ++whiteCount;
    // A black last key sticks out half its width (0.3 of a white key) past the
    // last white key; leave room for it.
    const float span = whiteCount + (count > 0 && isBlack(firstNote + count - 1) ? 0.3f : 0.0f);
    const float whiteWidth = size.x / span;

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    const bool pressed = ImGui::IsItemActive();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    // The key rectangles, in note order.
    KeyRect keys[kMaxKeys];
    for (int i = 0, white = 0; i < count; ++i) {
        const bool black = isBlack(firstNote + i);
        keys[i] = keyRect(white, black, origin, size, whiteWidth);
        if (!black) ++white;
    }

    // The mouse hits a black key first: black keys lie on top of the white ones.
    int mouseNote = -1;
    if (pressed) {
        for (int pass = 0; pass < 2 && mouseNote < 0; ++pass) {
            const bool wantBlack = pass == 0;
            for (int i = 0; i < count; ++i) {
                const KeyRect& k = keys[i];
                if (k.black == wantBlack && mouse.x >= k.min.x && mouse.x < k.max.x && mouse.y >= k.min.y &&
                    mouse.y < k.max.y) {
                    mouseNote = firstNote + i;
                    break;
                }
            }
        }
    }

    // White keys first, then the black keys over them.
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (int pass = 0; pass < 2; ++pass) {
        const bool drawBlack = pass == 1;
        for (int i = 0; i < count; ++i) {
            const KeyRect& k = keys[i];
            if (k.black != drawBlack) continue;
            const int note = firstNote + i;
            const bool lit = note < static_cast<int>(held.size()) && held[note] > 0;
            const ImU32 fill = lit ? color::kOrange : (k.black ? color::kKeyBlack : color::kKeyWhite);
            draw->AddRectFilled(k.min, k.max, fill, 6.0f, ImDrawFlags_RoundCornersBottom);
            if (k.black) draw->AddRect(k.min, k.max, color::kBackground, 6.0f, ImDrawFlags_RoundCornersBottom, 2.0f);
            if (labels[i] == 0) continue;
            const char text[2] = {labels[i], 0};
            const ImU32 ink = lit ? color::kBackground : (k.black ? color::kMuted : color::kDim);
            centeredText(draw, (k.min.x + k.max.x) / 2, k.max.y - ImGui::GetFontSize() - 10, ink, text);
        }
    }
    return mouseNote;
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
