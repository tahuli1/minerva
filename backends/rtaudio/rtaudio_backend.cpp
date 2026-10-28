#include "rtaudio/rtaudio_backend.h"

#include <RtAudio.h>

#include <algorithm>

namespace ukefx {

RtAudioBackend::RtAudioBackend()
    : audio_(std::make_unique<RtAudio>(RtAudio::WINDOWS_WASAPI)) {}

RtAudioBackend::~RtAudioBackend() { stop(); }

bool RtAudioBackend::open(const AudioConfig& config, ProcessFn process, void* user) {
    const unsigned int inId = audio_->getDefaultInputDevice();
    const unsigned int outId = audio_->getDefaultOutputDevice();
    if (inId == 0 || outId == 0) {
        lastError_ = inId == 0 ? "no default input (capture) device" : "no default output device";
        return false;
    }
    const RtAudio::DeviceInfo inInfo = audio_->getDeviceInfo(inId);
    const RtAudio::DeviceInfo outInfo = audio_->getDeviceInfo(outId);

    RtAudio::StreamParameters inParams{inId, 1, 0};
    RtAudio::StreamParameters outParams{outId, std::min(2u, outInfo.outputChannels), 0};

    process_ = process;
    user_ = user;
    outChannels_ = outParams.nChannels;

    // RtAudio may change bufferFrames to what the device supports; it writes the real value back.
    unsigned int bufferFrames = config.bufferFrames;
    if (audio_->openStream(&outParams, &inParams, RTAUDIO_FLOAT32, config.sampleRate,
                           &bufferFrames, &RtAudioBackend::audioCallback, this) != RTAUDIO_NO_ERROR) {
        lastError_ = audio_->getErrorText();
        return false;
    }

    // Allocate the scratch buffer here, before the stream starts: the callback must never allocate.
    mono_.assign(bufferFrames, 0.0f);

    info_.api = RtAudio::getApiDisplayName(audio_->getCurrentApi());
    info_.inputDevice = inInfo.name;
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

void RtAudioBackend::stop() {
    if (audio_->isStreamRunning()) audio_->stopStream();
    if (audio_->isStreamOpen()) audio_->closeStream();
}

// --- Audio thread. No allocation, locks, I/O or exceptions below this line. ---

int RtAudioBackend::audioCallback(void* out, void* in, unsigned int nFrames, double /*streamTime*/,
                                  unsigned int status, void* self) {
    auto* backend = static_cast<RtAudioBackend*>(self);
    auto* dst = static_cast<float*>(out);
    const unsigned int channels = backend->outChannels_;

    // status flags RtAudio's input overflow / output underflow. Relaxed is enough:
    // it's a standalone counter, nothing else is published through it.
    if (status != 0) backend->xruns_.fetch_add(1, std::memory_order_relaxed);

    // Never expected, but if the block is bigger than our buffer, output silence
    // rather than grow the buffer (allocation) or write out of bounds.
    if (in == nullptr || nFrames > backend->mono_.size()) {
        std::fill_n(dst, nFrames * channels, 0.0f);
        return 0;
    }

    float* mono = backend->mono_.data();
    backend->process_(static_cast<const float*>(in), mono, static_cast<int>(nFrames), backend->user_);

    // Output is interleaved: L R L R ...
    for (unsigned int i = 0; i < nFrames; ++i)
        for (unsigned int c = 0; c < channels; ++c)
            dst[i * channels + c] = mono[i];
    return 0;
}

} // namespace ukefx
