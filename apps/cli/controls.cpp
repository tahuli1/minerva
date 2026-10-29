#include "controls.h"

#include <algorithm>
#include <cstdio>

namespace cli {

void apply(const Settings& settings, ukefx::EffectSet& effects) {
    effects.distortion.setDrive(settings.drive);
    effects.delay.setFeedback(settings.feedback);
    effects.delay.setMix(settings.mix);
}

bool handleKey(int key, Settings& settings) {
    // Drive moves in ratios because the ear hears it on a log scale:
    // 1 -> 2 is as big a change as 10 -> 20.
    switch (key) {
    case 'd': settings.drive = std::max(1.0f, settings.drive / 1.25f); return true;
    case 'D': settings.drive = std::min(40.0f, settings.drive * 1.25f); return true;
    case 'f': settings.feedback = std::max(0.0f, settings.feedback - 0.05f); return true;
    case 'F': settings.feedback = std::min(0.95f, settings.feedback + 0.05f); return true;
    case 'm': settings.mix = std::max(0.0f, settings.mix - 0.05f); return true;
    case 'M': settings.mix = std::min(1.0f, settings.mix + 0.05f); return true;
    default: return false;
    }
}

void printHelp() {
    std::printf("Keys: d/D drive down/up   f/F delay feedback   m/M delay mix   q quit\n");
}

void printSettings(const Settings& settings) {
    // \r rewrites the same line, so the display stays one line while you play.
    std::printf("\rdrive %5.1f | feedback %.2f | mix %.2f   ", settings.drive, settings.feedback,
                settings.mix);
    std::fflush(stdout);
}

} // namespace cli
