#include "controls.h"

#include <algorithm>
#include <cstdio>

namespace cli {

namespace {

// Second code of F1..F6 from _getch() (after the 0 prefix). The letter keys are
// left free: in Minerva they play notes.
constexpr int kF1 = 59, kF2 = 60, kF3 = 61, kF4 = 62, kF5 = 63, kF6 = 64;

} // namespace

void apply(const Settings& settings, Effects& effects) {
    effects.distortion.setDrive(settings.drive);
    effects.delay.setFeedback(settings.feedback);
    effects.delay.setMix(settings.mix);
}

bool handleKey(int key, bool extended, Settings& settings) {
    if (!extended) return false;
    // Drive moves in ratios because the ear hears it on a log scale:
    // 1 -> 2 is as big a change as 10 -> 20.
    switch (key) {
    case kF1: settings.drive = std::max(1.0f, settings.drive / 1.25f); return true;
    case kF2: settings.drive = std::min(40.0f, settings.drive * 1.25f); return true;
    case kF3: settings.feedback = std::max(0.0f, settings.feedback - 0.05f); return true;
    case kF4: settings.feedback = std::min(0.95f, settings.feedback + 0.05f); return true;
    case kF5: settings.mix = std::max(0.0f, settings.mix - 0.05f); return true;
    case kF6: settings.mix = std::min(1.0f, settings.mix + 0.05f); return true;
    default: return false;
    }
}

void printHelp() {
    std::printf("Keys: F1/F2 drive down/up   F3/F4 delay feedback   F5/F6 delay mix   Esc quit\n");
}

void printSettings(const Settings& settings) {
    // \r rewrites the same line, so the display stays one line while you play.
    std::printf("\rdrive %5.1f | feedback %.2f | mix %.2f   ", settings.drive, settings.feedback,
                settings.mix);
    std::fflush(stdout);
}

} // namespace cli
