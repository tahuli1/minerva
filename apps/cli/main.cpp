#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <conio.h>

#include "controls.h"
#include "effect_chain.h"
#include "effect_set.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::atomic<bool> g_quit{false};

// Windows runs console control handlers on their own thread, so a plain atomic
// store is all that's needed; the main loop sees it within one poll.
BOOL WINAPI onConsoleCtrl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    g_quit.store(true);
    return TRUE; // Handled: don't let Windows kill the process before we close the stream.
}

// Audio thread. `user` is the effect to run; a virtual call has a small, fixed cost.
void runEffect(const float* in, float* out, int n, void* user) {
    static_cast<ukefx::Effect*>(user)->process(in, out, n);
}

// Main thread, not RT: polling the keyboard 50 times a second costs nothing and
// keeps the loop simple. Every change goes to the effects through their atomics.
void runControlLoop(cli::Settings& settings, ukefx::EffectSet& effects) {
    cli::printHelp();
    cli::printSettings(settings);
    while (!g_quit.load()) {
        if (!_kbhit()) {
            Sleep(20);
            continue;
        }
        const int key = _getch();
        if (key == 0 || key == 0xE0) { // arrow/function keys arrive as two codes: skip both
            _getch();
            continue;
        }
        if (key == 'q' || key == 'Q') break;
        if (cli::handleKey(key, settings)) {
            cli::apply(settings, effects);
            cli::printSettings(settings);
        }
    }
    std::printf("\n");
}

std::string join(const std::vector<std::string_view>& names) {
    std::string joined;
    for (std::string_view name : names) {
        if (!joined.empty()) joined += " -> ";
        joined += name;
    }
    return joined;
}

} // namespace

// Usage: ukefx_cli [effect ...]   effects run in the order given; known: distortion, delay
//        ukefx_cli                same as: ukefx_cli distortion delay
int main(int argc, char** argv) {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    ukefx::EffectSet effects;
    cli::Settings settings;
    cli::apply(settings, effects); // Before prepare(), so the smoothers start at these values.

    std::vector<std::string_view> names(argv + 1, argv + argc);
    if (names.empty()) names = {"distortion", "delay"};
    ukefx::EffectChain chain;
    if (const std::string error = ukefx::buildChain(effects, names, chain); !error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    // Declared after the effects so it is destroyed first: the stream stops
    // before the effects it calls into go away.
    ukefx::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, runEffect, &chain)) {
        std::fprintf(stderr, "Failed to open audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }

    const ukefx::StreamInfo& info = backend.info();
    std::printf("API:           %s\n", info.api.c_str());
    std::printf("Input device:  %s (1 channel)\n", info.inputDevice.c_str());
    std::printf("Output device: %s (%u channels)\n", info.outputDevice.c_str(), info.outputChannels);
    std::printf("Sample rate:   %u Hz\n", info.sampleRate);
    std::printf("Buffer size:   %u frames (%.2f ms)\n", info.bufferFrames,
                1000.0 * info.bufferFrames / info.sampleRate);
    std::printf("Reported stream latency: %ld frames\n", info.latencyFrames);
    std::printf("Effects:       %s\n", join(names).c_str());

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    chain.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    std::printf("Running. Press q or Ctrl+C to stop.\n");

    runControlLoop(settings, effects);

    backend.stop();
    std::printf("Stopped. xruns: %llu\n", static_cast<unsigned long long>(backend.xrunCount()));
    return 0;
}
