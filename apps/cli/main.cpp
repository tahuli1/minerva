#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <conio.h>

#include "controls.h"
#include "effects/effect_chain.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <cstdio>

namespace {

std::atomic<bool> g_quit{false};

constexpr int kEsc = 27;

// Windows runs console control handlers on their own thread, so a plain atomic
// store is all that's needed; the main loop sees it within one poll.
BOOL WINAPI onConsoleCtrl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    g_quit.store(true);
    return TRUE; // Handled: don't let Windows kill the process before we close the stream.
}

// Audio thread. `user` is the effect to run; a virtual call has a small, fixed cost.
void runEffect(const float* in, float* out, int n, void* user) {
    static_cast<minerva::Effect*>(user)->process(in, out, n);
}

// Main thread, not RT: polling the keyboard 50 times a second costs nothing and
// keeps the loop simple. Every change goes to the effects through their atomics.
void runControlLoop(cli::Settings& settings, cli::Effects& effects) {
    cli::printHelp();
    cli::printSettings(settings);
    while (!g_quit.load()) {
        if (!_kbhit()) {
            Sleep(20);
            continue;
        }
        int key = _getch();
        const bool extended = key == 0 || key == 0xE0; // function keys: a prefix, then the code
        if (extended) key = _getch();
        if (!extended && key == kEsc) break;
        if (cli::handleKey(key, extended, settings)) {
            cli::apply(settings, effects);
            cli::printSettings(settings);
        }
    }
    std::printf("\n");
}

} // namespace

// Until the synth is connected (Milestone 3c) the sound source is the mic, when
// there is one, through Minerva's fixed effect chain: distortion -> delay.
int main() {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    cli::Effects effects;
    cli::Settings settings;
    cli::apply(settings, effects);     // Before prepare(), so the smoothers start at these values.
    effects.distortion.setLevel(0.3f); // tanh saturates near full scale; keep headphones safe.

    minerva::EffectChain chain;
    chain.add(effects.distortion);
    chain.add(effects.delay);

    // Declared after the effects so it is destroyed first: the stream stops
    // before the effects it calls into go away.
    minerva::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, runEffect, &chain)) {
        std::fprintf(stderr, "Failed to open audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }

    const minerva::StreamInfo& info = backend.info();
    std::printf("API:           %s\n", info.api.c_str());
    if (info.inputDevice.empty())
        std::printf("Input device:  none (output only: silent until the synth arrives)\n");
    else
        std::printf("Input device:  %s (1 channel)\n", info.inputDevice.c_str());
    std::printf("Output device: %s (%u channels)\n", info.outputDevice.c_str(), info.outputChannels);
    std::printf("Sample rate:   %u Hz\n", info.sampleRate);
    std::printf("Buffer size:   %u frames (%.2f ms)\n", info.bufferFrames,
                1000.0 * info.bufferFrames / info.sampleRate);
    std::printf("Reported stream latency: %ld frames\n", info.latencyFrames);
    std::printf("Effects:       distortion -> delay\n");

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    chain.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    std::printf("Running. Press Esc or Ctrl+C to stop.\n");

    runControlLoop(settings, effects);

    backend.stop();
    std::printf("Stopped. xruns: %llu\n", static_cast<unsigned long long>(backend.xrunCount()));
    return 0;
}
