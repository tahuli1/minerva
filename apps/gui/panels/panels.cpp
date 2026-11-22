#include "panels/panels.h"

#include "keyboard/note_keys.h"
#include "widgets/widgets.h"

#include <algorithm>
#include <cstdio>

namespace gui {

namespace {

// A rounded panel the size of `size`; its contents follow until endPanel().
void beginPanel(const char* id, ImVec2 size) {
    ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
}
void endPanel() { ImGui::EndChild(); }

// A square button for octave down/up.
bool squareButton(const char* label) { return ImGui::Button(label, ImVec2(44, 44)); }

} // namespace

void drawHeader(const Fonts& fonts, const Texture& logo, const minerva::StreamInfo& info, int threadPriority) {
    constexpr float kLogo = kLogoSize;
    const float top = ImGui::GetCursorPosY();
    if (logo.view) {
        ImGui::Image(reinterpret_cast<ImTextureID>(logo.view.Get()), ImVec2(kLogo, kLogo));
        ImGui::SameLine(0, 14);
    }
    ImGui::PushFont(fonts.title);
    ImGui::SetCursorPosY(top + (kLogo - ImGui::GetFontSize()) / 2);
    ImGui::TextUnformatted("M I N E R V A");
    ImGui::PopFont();
    ImGui::SameLine(0, 16);
    ImGui::SetCursorPosY(top + (kLogo - ImGui::GetFontSize()) / 2 + 4);
    ImGui::TextDisabled("real-time synth");

    // The status on the right: audio on (green), the block, MMCSS.
    char block[64], mmcss[64];
    std::snprintf(block, sizeof block, "%u frames  %.2f ms", info.bufferFrames,
                  1000.0 * info.bufferFrames / info.sampleRate);
    std::snprintf(mmcss, sizeof mmcss, "MMCSS  priority %d", threadPriority);
    ImGui::PushFont(fonts.number);
    const float width = 28 + ImGui::CalcTextSize("AUDIO ON").x + 28 + ImGui::CalcTextSize(block).x + 28 +
                        ImGui::CalcTextSize(mmcss).x;
    const float y = top + (kLogo - ImGui::GetFontSize()) / 2;
    // SameLine() returns to the line's first height, so each item is put back on `y`.
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - width);
    ImGui::SetCursorPosY(y);
    const ImVec2 dot = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddCircleFilled(ImVec2(dot.x + 6, dot.y + ImGui::GetFontSize() / 2), 9, IM_COL32(127, 212, 154, 60));
    draw->AddCircleFilled(ImVec2(dot.x + 6, dot.y + ImGui::GetFontSize() / 2), 5, color::kGreen);
    ImGui::Dummy(ImVec2(16, ImGui::GetFontSize()));
    ImGui::SameLine(0, 12);
    ImGui::SetCursorPosY(y);
    ImGui::TextUnformatted("AUDIO ON");
    ImGui::SameLine(0, 28);
    ImGui::SetCursorPosY(y);
    ImGui::TextDisabled("%s", block);
    ImGui::SameLine(0, 28);
    ImGui::SetCursorPosY(y);
    ImGui::TextDisabled("%s", mmcss);
    ImGui::PopFont();
    ImGui::SetCursorPosY(top + kLogo + 8);
}

Changed drawSoundPanel(app::Settings& settings, ImVec2 size) {
    Changed changed;
    beginPanel("sound", size);
    sectionTitle("S O U N D");

    struct Pad {
        const char* label;
        WaveShape shape;
        minerva::Waveform waveform;
    };
    const Pad pads[] = {{"sine", WaveShape::Sine, minerva::Waveform::Sine},
                        {"triangle", WaveShape::Triangle, minerva::Waveform::Triangle},
                        {"saw", WaveShape::Saw, minerva::Waveform::Saw},
                        {"square", WaveShape::Square, minerva::Waveform::Square}};
    const float padWidth = (ImGui::GetContentRegionAvail().x - 3 * 10) / 4;
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine(0, 10);
        if (waveformPad(pads[i].label, pads[i].label, pads[i].shape, settings.waveform == pads[i].waveform,
                        ImVec2(padWidth, 76))) {
            settings.waveform = pads[i].waveform;
            changed.waveform = changed.settings = true;
        }
    }

    // Octave on the left, velocity on the right. SameLine() returns to the line's
    // first height, so every item is placed on an explicit height.
    const float half = (ImGui::GetContentRegionAvail().x - 12) / 2;
    const float labelY = ImGui::GetCursorPosY();
    ImGui::TextDisabled("OCTAVE");
    const float rowY = ImGui::GetCursorPosY();
    if (squareButton("-##oct")) {
        settings.octave = (std::max)(app::NoteKeys::kMinOctave, settings.octave - 1);
        changed.settings = true;
    }
    ImGui::SameLine(0, 14);
    ImGui::SetCursorPosY(rowY + (44 - ImGui::GetFontSize()) / 2);
    ImGui::Text("%d", settings.octave);
    ImGui::SameLine(0, 14);
    ImGui::SetCursorPosY(rowY);
    if (squareButton("+##oct")) {
        settings.octave = (std::min)(app::NoteKeys::kMaxOctave, settings.octave + 1);
        changed.settings = true;
    }
    // SetCursorPos, not SameLine: SameLine would also carry the buttons' text offset.
    ImGui::SetCursorPos(ImVec2(half + 24, labelY));
    ImGui::BeginGroup();
    ImGui::TextDisabled("VELOCITY");
    ImGui::SetCursorPosY(rowY + (44 - ImGui::GetFrameHeight()) / 2);
    ImGui::SetNextItemWidth(half - 12);
    if (ImGui::SliderFloat("##velocity", &settings.velocity, 0.1f, 1.0f, "%.1f")) changed.settings = true;
    ImGui::EndGroup();
    endPanel();
    return changed;
}

Changed drawEffectsPanel(app::Settings& settings, ImVec2 size) {
    Changed changed;
    beginPanel("effects", size);
    sectionTitle("E F F E C T S");
    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize("DISTORTION OFF").x - 32);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 8);
    if (pillToggle("dist", settings.distortion ? "DISTORTION ON" : "DISTORTION OFF", settings.distortion)) {
        settings.distortion = !settings.distortion;
        changed.settings = true;
    }

    const float column = ImGui::GetContentRegionAvail().x / 3;
    const float start = ImGui::GetCursorPosX();
    struct Knob {
        const char* id;
        const char* label;
        float* value;
        float min, max;
        bool enabled;
    };
    const Knob knobs[] = {{"drive", "DRIVE", &settings.drive, 0.05f, 0.95f, settings.distortion},
                          {"feedback", "FEEDBACK", &settings.feedback, 0.0f, 0.95f, true},
                          {"mix", "MIX", &settings.mix, 0.0f, 1.0f, true}};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine();
        ImGui::SetCursorPosX(start + column * i + (column - 92) / 2);
        if (knob(knobs[i].id, knobs[i].label, knobs[i].value, knobs[i].min, knobs[i].max, knobs[i].enabled))
            changed.settings = true;
    }
    endPanel();
    return changed;
}

void drawEnginePanel(const Fonts& fonts, const minerva::TimingSummary& timing, ImVec2 size) {
    beginPanel("engine", size);
    sectionTitle("E N G I N E");
    const float card = (ImGui::GetContentRegionAvail().x - 10) / 2;
    char maxText[32], loadText[32], gapText[32], lateText[32];
    const double load = timing.deadlineMs > 0 ? 100.0 * timing.maxMs / timing.deadlineMs : 0.0;
    std::snprintf(maxText, sizeof maxText, "%.0f us", 1000.0 * timing.maxMs);
    std::snprintf(loadText, sizeof loadText, "%.1f %%", load);
    std::snprintf(gapText, sizeof gapText, "%.2f ms", timing.maxGapMs);
    std::snprintf(lateText, sizeof lateText, "%d", timing.lateCallbacks);
    const ImVec2 cardSize(card, 72);
    statCard("CALLBACK MAX", maxText, color::kText, cardSize, fonts.bigNumber);
    ImGui::SameLine(0, 10);
    statCard("DEADLINE USED", loadText, color::kText, cardSize, fonts.bigNumber);
    statCard("GAP MAX", gapText, color::kText, cardSize, fonts.bigNumber);
    ImGui::SameLine(0, 10);
    statCard("LATE CALLBACKS", lateText, timing.lateCallbacks == 0 ? color::kGreen : color::kOrange, cardSize,
             fonts.bigNumber);
    endPanel();
}

} // namespace gui
