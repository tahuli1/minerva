#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "delay.h"
#include "distortion.h"
#include "effect_chain.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

std::atomic<bool> g_quit{false};

// Windows runs console control handlers on their own thread (not inside a signal
// context), so using an atomic's wait/notify here is safe.
BOOL WINAPI onConsoleCtrl(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) return FALSE;
    g_quit.store(true);
    g_quit.notify_one();
    return TRUE; // Handled: don't let Windows kill the process before we close the stream.
}

// Audio thread. `user` is the effect to run; a virtual call has a small, fixed cost.
void runEffect(const float* in, float* out, int n, void* user) {
    static_cast<ukefx::Effect*>(user)->process(in, out, n);
}

struct Effects {
    ukefx::Distortion distortion;
    ukefx::Delay delay;
};

// Not RT. Adds the effects named on the command line to the chain, in order.
// Each effect object can appear once: adding it twice would make both slots share
// one set of state (e.g. one delay buffer written twice per sample).
bool buildChain(int argc, char** argv, Effects& effects, ukefx::EffectChain& chain, std::string& names) {
    bool used[2] = {false, false};
    for (int i = 1; i < argc; ++i) {
        const bool isDistortion = std::strcmp(argv[i], "distortion") == 0;
        const bool isDelay = std::strcmp(argv[i], "delay") == 0;
        if (!isDistortion && !isDelay) {
            std::fprintf(stderr, "Unknown effect '%s'. Known effects: distortion, delay\n", argv[i]);
            return false;
        }
        bool& wasUsed = used[isDelay ? 1 : 0];
        if (wasUsed) {
            std::fprintf(stderr, "Effect '%s' can only appear once\n", argv[i]);
            return false;
        }
        wasUsed = true;
        chain.add(isDelay ? static_cast<ukefx::Effect&>(effects.delay) : effects.distortion);
        names += names.empty() ? argv[i] : std::string(" -> ") + argv[i];
    }
    return true;
}

} // namespace

// Usage: ukefx_cli [effect ...]   effects run in the order given; known: distortion, delay
//        ukefx_cli                same as: ukefx_cli distortion delay
int main(int argc, char** argv) {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    Effects effects;
    effects.distortion.setDrive(4.0f);  // Warm overdrive rather than fuzz.
    effects.distortion.setLevel(0.3f);  // tanh saturates near full scale; keep headphones safe.
                                        // The delay keeps its defaults: a musical echo, see delay.h.
    ukefx::EffectChain chain;
    std::string names;
    char* defaultArgs[] = {argv[0], const_cast<char*>("distortion"), const_cast<char*>("delay")};
    const bool ok = argc > 1 ? buildChain(argc, argv, effects, chain, names)
                             : buildChain(3, defaultArgs, effects, chain, names);
    if (!ok) return 1;

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
    std::printf("Effects:       %s\n", names.c_str());

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // This is where the delay allocates its 2-second buffer.
    chain.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

    if (!backend.start()) {
        std::fprintf(stderr, "Failed to start audio stream: %s\n", backend.lastError().c_str());
        return 1;
    }
    std::printf("Running. Press Ctrl+C to stop.\n");

    g_quit.wait(false); // Blocks without polling until the Ctrl+C handler flips the flag.

    backend.stop();
    std::printf("Stopped. xruns: %llu\n", static_cast<unsigned long long>(backend.xrunCount()));
    return 0;
}
