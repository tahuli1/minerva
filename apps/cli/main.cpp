#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "display/display.h"
#include "engine/engine.h"
#include "keyboard/console_keyboard.h"
#include "keyboard/controls.h"
#include "keyboard/note_keys.h"
#include "measurement/cpu_load.h"
#include "measurement/timing_csv.h"
#include "options.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>

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
// once a second. With --seconds it stops by itself, for unattended runs.
void runControlLoop(minerva::Engine& engine, cli::Settings& settings,
                    minerva::RtAudioBackend& backend, const cli::Options& options,
                    cli::TimingCsv& csv) {
    cli::ConsoleKeyboard keyboard;
    cli::NoteKeys noteKeys;
    minerva::TimingStats stats(backend.ticksPerSecond(), static_cast<int>(backend.info().sampleRate));
    cli::TimingTable table;
    const auto start = std::chrono::steady_clock::now();
    auto nextReport = start + kReportPeriod;
    if (options.showStats) table.printTitle();
    else std::printf("\n");
    cli::printSettings(settings);
    while (!g_quit.load()) {
        if (options.seconds > 0 && std::chrono::steady_clock::now() - start >= std::chrono::seconds(options.seconds))
            return;
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
        while (backend.popTiming(timing)) {
            stats.add(timing);
            csv.write(timing);
        }
        if (std::chrono::steady_clock::now() >= nextReport) {
            if (options.showStats) {
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
    cli::Options options;
    std::string error;
    if (!cli::parseOptions(argc, argv, options, error)) {
        std::fprintf(stderr, "%s\n%s\n", error.c_str(), cli::usage());
        return 1;
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
    if (!backend.open({.sampleRate = 48000, .bufferFrames = options.bufferFrames}, runEngine, &engine)) {
        std::fprintf(stderr, "Failed to open audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }

    cli::TimingCsv csv;
    if (!options.csvPath.empty() && !csv.open(options.csvPath, backend.ticksPerSecond())) {
        std::fprintf(stderr, "Cannot write %s\n", options.csvPath.c_str());
        return 1;
    }

    const minerva::StreamInfo& info = backend.info();
    cli::printTitle();
    cli::printAudio(info);

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    engine.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    // Started before the audio, so every measured callback runs under the same load.
    cli::CpuLoad load(options.loadThreads);

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    // The audio thread reads its priority on its first callback; wait for it, up to a second.
    for (int i = 0; i < 50 && backend.audioThreadPriority() == minerva::RtAudioBackend::kPriorityUnknown; ++i)
        Sleep(20);
    cli::printAudioThread(backend.audioThreadPriority(), options.loadThreads);
    cli::printKeys(options.showStats);

    runControlLoop(engine, settings, backend, options, csv);

    backend.stop();
    std::printf("\n\nStopped.\n");
    return 0;
}
