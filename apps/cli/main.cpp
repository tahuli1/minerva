#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "console_keyboard.h"
#include "controls.h"
#include "display.h"
#include "engine/engine.h"
#include "note_keys.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>

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
// It also drains the callback timings, with or without --stats, so the audio
// thread's work is the same either way; with --stats it adds a row to the table
// once a second.
void runControlLoop(minerva::Engine& engine, cli::Settings& settings,
                    minerva::RtAudioBackend& backend, bool showStats) {
    cli::ConsoleKeyboard keyboard;
    cli::NoteKeys noteKeys;
    minerva::TimingStats stats(backend.ticksPerSecond(), static_cast<int>(backend.info().sampleRate));
    cli::TimingTable table;
    auto nextReport = std::chrono::steady_clock::now() + kReportPeriod;
    if (showStats) table.printTitle();
    else std::printf("\n");
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
            if (showStats) {
                table.printRow(stats.summary(), backend.droppedTimings());
                cli::printSettings(settings);
            }
            stats.reset();
            nextReport += kReportPeriod;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    // --stats shows the callback timing table; without it the screen is just for playing.
    bool showStats = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--stats") == 0) {
            showStats = true;
        } else {
            std::fprintf(stderr, "Unknown option: %s\nUsage: minerva_cli [--stats]\n", argv[i]);
            return 1;
        }
    }

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
    cli::printTitle();
    cli::printAudio(info);
    cli::printKeys(showStats);

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    engine.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    runControlLoop(engine, settings, backend, showStats);

    backend.stop();
    std::printf("\n\nStopped.\n");
    return 0;
}
