#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class RtAudio;

namespace ukefx {

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
    std::string inputDevice;
    std::string outputDevice;
    unsigned int sampleRate = 0;
    unsigned int bufferFrames = 0;
    unsigned int outputChannels = 0;
    long latencyFrames = 0;
};

// Duplex stream on the default input/output devices via RtAudio (WASAPI).
// The ukulele is a mono source: we capture one channel, process it mono,
// and copy the result to every output channel.
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
    std::uint64_t xrunCount() const { return xruns_.load(std::memory_order_relaxed); }

private:
    static int audioCallback(void* out, void* in, unsigned int nFrames, double streamTime,
                             unsigned int status, void* self);

    std::unique_ptr<RtAudio> audio_;
    ProcessFn process_ = nullptr;
    void* user_ = nullptr;
    std::vector<float> mono_; // Sized in open(), never resized while running.
    unsigned int outChannels_ = 0;
    std::atomic<std::uint64_t> xruns_{0};
    StreamInfo info_;
    std::string lastError_;

    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
};

} // namespace ukefx
