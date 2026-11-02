#include "rtaudio/rtaudio_backend.h"

#include "denormals.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <RtAudio.h>

#include <algorithm>

namespace minerva {

namespace {

// QueryPerformanceCounter: Windows' high-resolution clock (well under a
// microsecond), read without blocking, so the audio thread may call it.
std::int64_t nowTicks() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

} // namespace

RtAudioBackend::RtAudioBackend()
    : audio_(std::make_unique<RtAudio>(RtAudio::WINDOWS_WASAPI)) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    ticksPerSecond_ = static_cast<double>(frequency.QuadPart);
}

RtAudioBackend::~RtAudioBackend() { stop(); }

bool RtAudioBackend::open(const AudioConfig& config, ProcessFn process, void* user) {
    const unsigned int outId = audio_->getDefaultOutputDevice();
    if (outId == 0) {
        lastError_ = "no default output device";
        return false;
    }
    const RtAudio::DeviceInfo outInfo = audio_->getDeviceInfo(outId);
    RtAudio::StreamParameters outParams{outId, std::min(2u, outInfo.outputChannels), 0};

    process_ = process;
    user_ = user;
    outChannels_ = outParams.nChannels;

    // RtAudio may change bufferFrames to what the device supports; it writes the real value back.
    unsigned int bufferFrames = config.bufferFrames;
    if (audio_->openStream(&outParams, nullptr, RTAUDIO_FLOAT32, config.sampleRate,
                           &bufferFrames, &RtAudioBackend::audioCallback, this) != RTAUDIO_NO_ERROR) {
        lastError_ = audio_->getErrorText();
        return false;
    }

    // Allocate the scratch buffers here, before the stream starts: the callback must never allocate.
    mono_.assign(bufferFrames, 0.0f);
    silence_.assign(bufferFrames, 0.0f);

    info_.api = RtAudio::getApiDisplayName(audio_->getCurrentApi());
    info_.outputDevice = outInfo.name;
    info_.sampleRate = audio_->getStreamSampleRate();
    info_.bufferFrames = bufferFrames;
    info_.outputChannels = outChannels_;
    info_.latencyFrames = audio_->getStreamLatency();
    return true;
}

bool RtAudioBackend::start() {
    if (audio_->startStream() != RTAUDIO_NO_ERROR) {
        lastError_ = audio_->getErrorText();
        return false;
    }
    return true;
}

bool RtAudioBackend::popTiming(CallbackTiming& timing) {
    const CallbackTiming* next = timings_.front();
    if (next == nullptr) return false;
    timing = *next;
    timings_.pop();
    return true;
}

void RtAudioBackend::stop() {
    if (audio_->isStreamRunning()) audio_->stopStream();
    if (audio_->isStreamOpen()) audio_->closeStream();
}

// --- Audio thread. No allocation, locks, I/O or exceptions below this line. ---

// `status` is not used: RtAudio's WASAPI code only sets it when captured input is
// lost, and there is no input. WASAPI does not report output underruns to the app
// at all, so lateness is judged from the callback timings instead.
int RtAudioBackend::audioCallback(void* out, void* /*in*/, unsigned int nFrames,
                                  double /*streamTime*/, unsigned int /*status*/, void* self) {
    const std::int64_t start = nowTicks();
    // Set per callback rather than once: RtAudio owns this thread, and this way we
    // don't depend on nothing else having changed MXCSR in between. Costs a few cycles.
    ScopedNoDenormals noDenormals;

    auto* backend = static_cast<RtAudioBackend*>(self);
    auto* dst = static_cast<float*>(out);
    const unsigned int channels = backend->outChannels_;

    // Never expected, but if the block is bigger than our buffers, output silence
    // rather than grow them (allocation) or write out of bounds.
    if (nFrames > backend->mono_.size()) {
        std::fill_n(dst, nFrames * channels, 0.0f);
        return 0;
    }

    float* mono = backend->mono_.data();
    backend->process_(backend->silence_.data(), mono, static_cast<int>(nFrames), backend->user_);

    // Output is interleaved: L R L R ...
    for (unsigned int i = 0; i < nFrames; ++i)
        for (unsigned int c = 0; c < channels; ++c)
            dst[i * channels + c] = mono[i];

    const CallbackTiming timing{start, nowTicks() - start, nFrames};
    if (!backend->timings_.try_push(timing))
        backend->droppedTimings_.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

} // namespace minerva
