#include "display.h"

#include "synth/synth.h"

#include <cstdio>

namespace cli {

namespace {

// Wider than any line we print: clearing this many columns wipes the NOW line,
// which has no newline, before something else is printed over it.
constexpr int kLineWidth = 120;

void clearLine() { std::printf("\r%*s\r", kLineWidth, ""); }

void printSection(const char* name) { std::printf("\n%s\n", name); }

// One aligned "label  value" row of the AUDIO and KEYS sections.
void printField(const char* label, const char* value) { std::printf("  %-13s %s\n", label, value); }

} // namespace

void printTitle() {
    std::printf("=== Minerva =====================================================================\n");
}

void printAudio(const minerva::StreamInfo& info) {
    char text[160];
    printSection("AUDIO");
    printField("API", info.api.c_str());
    std::snprintf(text, sizeof text, "%s, %u channels", info.outputDevice.c_str(), info.outputChannels);
    printField("Output", text);
    if (info.inputDevice.empty()) {
        printField("Input", "none (output only)");
    } else {
        std::snprintf(text, sizeof text, "%s (unused until the looper)", info.inputDevice.c_str());
        printField("Input", text);
    }
    std::snprintf(text, sizeof text, "%u Hz", info.sampleRate);
    printField("Sample rate", text);
    std::snprintf(text, sizeof text, "%u frames = %.2f ms (the deadline)", info.bufferFrames,
                  1000.0 * info.bufferFrames / info.sampleRate);
    printField("Block", text);
    std::snprintf(text, sizeof text, "%ld frames (as reported by the driver)", info.latencyFrames);
    printField("Latency", text);
    std::snprintf(text, sizeof text, "%d-voice synth -> distortion -> delay", minerva::Synth::kVoices);
    printField("Sound", text);
}

void printKeys() {
    printSection("KEYS");
    printField("Play", "A S D F G H J K L ;  white keys    W E T Y U O P  black keys");
    printField("Sound", "Z/X octave    C/V velocity    1-4 waveform");
    printField("Effects", "F7 distortion on/off   F1/F2 drive   F3/F4 feedback   F5/F6 mix");
    printField("Quit", "Esc (or Ctrl+C)");
}

void TimingTable::printTitle() {
    clearLine();
    printSection("CALLBACK TIMING (one row per second)");
    printHeader();
}

void TimingTable::printHeader() {
    clearLine();
    std::printf("  time |  avg us | p99.9 us |  max us | load %% | gap max ms | xruns | dropped\n");
    std::printf("-------+---------+----------+---------+--------+------------+-------+--------\n");
}

void TimingTable::printRow(const minerva::TimingSummary& s, std::uint64_t droppedTimings) {
    if (rows_ > 0 && rows_ % kRowsPerHeader == 0) printHeader();
    ++rows_;
    // load: the slowest callback as a share of the deadline, i.e. how much of the
    // block period the worst case used.
    const double load = s.deadlineMs > 0.0 ? 100.0 * s.maxMs / s.deadlineMs : 0.0;
    clearLine();
    std::printf("%5ds | %7.1f | %8.1f | %7.1f | %6.1f | %10.2f | %5d | %7llu\n", rows_,
                1000.0 * s.avgMs, 1000.0 * s.p999Ms, 1000.0 * s.maxMs, load, s.maxGapMs,
                s.driverXruns, static_cast<unsigned long long>(droppedTimings));
}

void printSettings(const Settings& settings) {
    clearLine();
    std::printf("NOW  octave %d | velocity %.1f | %s | distortion %s (drive %.2f) | "
                "delay feedback %.2f, mix %.2f",
                settings.octave, settings.velocity, waveformName(settings.waveform),
                settings.distortion ? "on" : "off", settings.drive, settings.feedback, settings.mix);
    std::fflush(stdout);
}

} // namespace cli
