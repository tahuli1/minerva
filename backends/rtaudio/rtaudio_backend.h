#pragma once

#include "rt/callback_timing.h"

#include <rigtorp/SPSCQueue.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class RtAudio;

namespace minerva {

// Mono in, mono out, n frames. Runs on the audio thread, so it must follow the RT rules.
// A plain function pointer (not std::function) so calling it can never allocate.
using ProcessFn = void (*)(const float* in, float* out, int n, void* user);

struct AudioConfig {
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 128;
};

// What the device actually gave us; may differ from what we asked for.
struct StreamInfo {
    std::string api;
    std::string outputDevice;
    unsigned int sampleRate = 0;
    unsigned int bufferFrames = 0;
    unsigned int outputChannels = 0;
    long latencyFrames = 0;
};

// Output-only audio stream on the default device via RtAudio (WASAPI). Processing is
// mono and the result is copied to every output channel. The mic is not opened:
// nothing uses it yet (it would come back with the looper, a bonus milestone), and
// an open mic shows the Windows "microphone in use" icon. `in` is a block of
// silence, so the process function never has to check.
class RtAudioBackend {
public:
    RtAudioBackend();
    ~RtAudioBackend();
    RtAudioBackend(const RtAudioBackend&) = delete;
    RtAudioBackend& operator=(const RtAudioBackend&) = delete;

    // Not RT: allocates and talks to the driver. Returns false and sets lastError() on failure.
    bool open(const AudioConfig& config, ProcessFn process, void* user);
    bool start();
    void stop(); // Stops and closes the stream; safe to call more than once.

    const StreamInfo& info() const { return info_; }
    const std::string& lastError() const { return lastError_; }

    // Main thread. Every callback is timed, and the timing travels to the main
    // thread through an SPSC queue: the audio thread writes, the main thread reads.
    // Returns false when no timing is waiting.
    bool popTiming(CallbackTiming& timing);
    // Timings the audio thread had to throw away because the queue was full: it
    // never waits for the main thread, so a slow reader loses data, not deadlines.
    std::uint64_t droppedTimings() const { return droppedTimings_.load(std::memory_order_relaxed); }
    // The rate of the clock the timings are in (QueryPerformanceFrequency).
    double ticksPerSecond() const { return ticksPerSecond_; }

private:
    static int audioCallback(void* out, void* in, unsigned int nFrames, double streamTime,
                             unsigned int status, void* self);

    std::unique_ptr<RtAudio> audio_;
    ProcessFn process_ = nullptr;
    void* user_ = nullptr;
    std::vector<float> mono_;    // Sized in open(), never resized while running.
    std::vector<float> silence_; // Passed as `in`, since there is no mic.
    unsigned int outChannels_ = 0;

    // About 11 s of 128-frame blocks at 48 kHz, so the main thread can stall for a
    // long time before anything is lost. Allocated here, before audio starts.
    static constexpr std::size_t kTimingCapacity = 4096;
    rigtorp::SPSCQueue<CallbackTiming> timings_{kTimingCapacity};
    std::atomic<std::uint64_t> droppedTimings_{0};
    double ticksPerSecond_ = 0.0;
    StreamInfo info_;
    std::string lastError_;

    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
};

} // namespace minerva
