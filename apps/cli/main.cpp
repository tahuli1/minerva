#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "console_keyboard.h"
#include "controls.h"
#include "engine/engine.h"
#include "note_keys.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <chrono>
#include <cstdio>

namespace {

std::atomic<bool> g_quit{false};

// How long the keyboard wait may sleep before checking g_quit. It only bounds how
// long Ctrl+C takes to be noticed; keys wake the wait at once.
constexpr unsigned long kWaitMs = 100;

// Windows runs console control handlers on their own thread, so a plain atomic
// store is all that's needed; the main loop sees it within one wait.
BOOL WINAPI onConsoleCtrl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    g_quit.store(true);
    return TRUE; // Handled: don't let Windows kill the process before we close the stream.
}

// How often the main thread summarizes the callback timings.
constexpr std::chrono::seconds kReportPeriod{1};

// Audio thread. `user` is the engine.
void runEngine(const float* in, float* out, int n, void* user) {
    static_cast<minerva::Engine*>(user)->process(in, out, n);
}

// Main thread, not RT. It sleeps until a key arrives, then turns it into commands
// for the engine. noteOn()/noteOff() never block; their queue holds 256 commands,
// far more than hands can press between two audio blocks, so they are not checked.
// Once a second it also drains the callback timings and prints their summary.
void runControlLoop(minerva::Engine& engine, cli::Settings& settings,
                    minerva::RtAudioBackend& backend) {
    cli::ConsoleKeyboard keyboard;
    cli::NoteKeys noteKeys;
    minerva::TimingStats stats(backend.ticksPerSecond(), static_cast<int>(backend.info().sampleRate));
    auto nextReport = std::chrono::steady_clock::now() + kReportPeriod;
    cli::printHelp();
    cli::printSettings(settings);
    while (!g_quit.load()) {
        for (const cli::KeyEvent& e : keyboard.wait(kWaitMs)) {
            switch (e.type) {
            case cli::KeyEvent::Type::Down:
                if (e.key == VK_ESCAPE) return;
                if (auto note = noteKeys.press(e.key, settings.octave)) {
                    engine.noteOn(*note, settings.velocity);
                } else {
                    const minerva::Waveform waveform = settings.waveform;
                    if (!cli::handleKey(e.key, settings)) break;
                    if (settings.waveform != waveform) engine.setWaveform(settings.waveform);
                    cli::apply(settings, engine);
                    cli::printSettings(settings);
                }
                break;
            case cli::KeyEvent::Type::Up:
                if (auto note = noteKeys.release(e.key)) engine.noteOff(*note);
                break;
            case cli::KeyEvent::Type::FocusLost:
                // The key-ups now go to another window; stop every note so none is stuck.
                for (int note : noteKeys.releaseAll()) engine.noteOff(note);
                break;
            }
        }

        minerva::CallbackTiming timing;
        while (backend.popTiming(timing)) stats.add(timing);
        if (std::chrono::steady_clock::now() >= nextReport) {
            cli::printTiming(stats.summary(), backend.droppedTimings());
            cli::printSettings(settings);
            stats.reset();
            nextReport += kReportPeriod;
        }
    }
}

} // namespace

int main() {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    minerva::Engine engine;
    cli::Settings settings;
    cli::apply(settings, engine);       // Before prepare(), so the smoothers start at these values.
    engine.setWaveform(settings.waveform);
    engine.distortion().setLevel(0.3f); // the overdrive peaks near full scale; keep headphones safe.

    // Declared after the engine so it is destroyed first: the stream stops before
    // the engine it calls into goes away.
    minerva::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, runEngine, &engine)) {
        std::fprintf(stderr, "Failed to open audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }

    const minerva::StreamInfo& info = backend.info();
    std::printf("API:           %s\n", info.api.c_str());
    if (info.inputDevice.empty())
        std::printf("Input device:  none (output only)\n");
    else
        std::printf("Input device:  %s (1 channel, unused until the looper)\n", info.inputDevice.c_str());
    std::printf("Output device: %s (%u channels)\n", info.outputDevice.c_str(), info.outputChannels);
    std::printf("Sample rate:   %u Hz\n", info.sampleRate);
    std::printf("Buffer size:   %u frames (%.2f ms)\n", info.bufferFrames,
                1000.0 * info.bufferFrames / info.sampleRate);
    std::printf("Reported stream latency: %ld frames\n", info.latencyFrames);
    std::printf("Sound:         %d-voice synth -> distortion -> delay\n", minerva::Synth::kVoices);

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    engine.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    std::printf("Running. Press Esc or Ctrl+C to stop.\n");

    runControlLoop(engine, settings, backend);

    backend.stop();
    std::printf("\nStopped. xruns: %llu\n", static_cast<unsigned long long>(backend.xrunCount()));
    return 0;
}
