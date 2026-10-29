#include "controls.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdio>

namespace cli {

namespace {

constexpr float kVelocityStep = 0.1f;
constexpr float kMinVelocity = 0.1f; // quieter than this is hard to hear at all

const char* name(minerva::Waveform waveform) {
    switch (waveform) {
    case minerva::Waveform::Sine: return "sine";
    case minerva::Waveform::Triangle: return "triangle";
    case minerva::Waveform::Saw: return "saw";
    case minerva::Waveform::Square: return "square";
    }
    return "?";
}

} // namespace

void apply(const Settings& settings, minerva::Engine& engine) {
    engine.distortion().setEnabled(settings.distortion);
    engine.distortion().setDrive(settings.drive);
    engine.delay().setFeedback(settings.feedback);
    engine.delay().setMix(settings.mix);
    engine.setWaveform(settings.waveform);
}

bool handleKey(int key, Settings& settings) {
    using minerva::Distortion;
    using minerva::Waveform;
    switch (key) {
    case 'Z': settings.octave = std::max(NoteKeys::kMinOctave, settings.octave - 1); return true;
    case 'X': settings.octave = std::min(NoteKeys::kMaxOctave, settings.octave + 1); return true;
    case 'C': settings.velocity = std::max(kMinVelocity, settings.velocity - kVelocityStep); return true;
    case 'V': settings.velocity = std::min(1.0f, settings.velocity + kVelocityStep); return true;
    case '1': settings.waveform = Waveform::Sine; return true;
    case '2': settings.waveform = Waveform::Triangle; return true;
    case '3': settings.waveform = Waveform::Saw; return true;
    case '4': settings.waveform = Waveform::Square; return true;
    case VK_F1: settings.drive = std::max(Distortion::kMinDrive, settings.drive - 0.05f); return true;
    case VK_F2: settings.drive = std::min(Distortion::kMaxDrive, settings.drive + 0.05f); return true;
    case VK_F3: settings.feedback = std::max(0.0f, settings.feedback - 0.05f); return true;
    case VK_F4: settings.feedback = std::min(0.95f, settings.feedback + 0.05f); return true;
    case VK_F5: settings.mix = std::max(0.0f, settings.mix - 0.05f); return true;
    case VK_F6: settings.mix = std::min(1.0f, settings.mix + 0.05f); return true;
    case VK_F7: settings.distortion = !settings.distortion; return true;
    default: return false;
    }
}

void printHelp() {
    std::printf("Play:  A S D F G H J K L ;  (white keys)   W E T Y U O P  (black keys)\n");
    std::printf("Keys:  Z/X octave   C/V velocity   1-4 sine/triangle/saw/square\n");
    std::printf("       F7 distortion on/off   F1/F2 drive   F3/F4 delay feedback   F5/F6 delay mix\n");
    std::printf("       Esc quit\n");
}

void printSettings(const Settings& settings) {
    // \r rewrites the same line, so the display stays one line while you play.
    std::printf("\roctave %d | velocity %.1f | %-8s | distortion %-3s drive %.2f | "
                "feedback %.2f | mix %.2f   ",
                settings.octave, settings.velocity, name(settings.waveform),
                settings.distortion ? "on" : "off", settings.drive, settings.feedback, settings.mix);
    std::fflush(stdout);
}

} // namespace cli
