#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "delay.h"
#include "distortion.h"
#include "rtaudio/rtaudio_backend.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

} // namespace

// Usage: ukefx_cli [drive]   distortion (default drive 10; 1 is almost clean)
//        ukefx_cli delay     delay, 350 ms, feedback 0.4, mix 0.35
// Until the effect chain exists, only one effect runs at a time.
int main(int argc, char** argv) {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    const bool useDelay = argc > 1 && std::strcmp(argv[1], "delay") == 0;
    const float drive = argc > 1 && !useDelay ? std::strtof(argv[1], nullptr) : 10.0f;

    ukefx::Distortion distortion;
    distortion.setDrive(drive);
    distortion.setLevel(0.3f); // tanh saturates near full scale; keep headphones safe.
    ukefx::Delay delay;        // Defaults are a musical echo; see delay.h.

    ukefx::Effect& effect = useDelay ? static_cast<ukefx::Effect&>(delay) : distortion;

    // Declared after the effects so it is destroyed first: the stream stops
    // before the effect it calls into goes away.
    ukefx::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, runEffect, &effect)) {
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
    if (useDelay)
        std::printf("Effect:        delay, 350 ms, feedback 0.4, mix 0.35\n");
    else
        std::printf("Effect:        distortion, drive %.1f\n", drive);

    // Prepare with the buffer size the device actually gave us, before audio starts.
    // For the delay this is where its 2-second buffer is allocated.
    effect.prepare(static_cast<int>(info.sampleRate), static_cast<int>(info.bufferFrames));

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
