#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "rtaudio/rtaudio_backend.h"

#include <algorithm>
#include <atomic>
#include <cstdio>

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

// Audio thread. Milestone 1 is a plain pass-through; the effect chain goes here later.
void passThrough(const float* in, float* out, int n, void* /*user*/) {
    std::copy_n(in, n, out);
}

} // namespace

int main() {
    SetConsoleCtrlHandler(onConsoleCtrl, TRUE);

    ukefx::RtAudioBackend backend;
    if (!backend.open({.sampleRate = 48000, .bufferFrames = 128}, passThrough, nullptr)) {
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
