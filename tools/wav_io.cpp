#include "wav_io.h"

// dr_wav is header-only: exactly one source file in the program must ask for its
// implementation. This is that file.
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

namespace ukefx {

std::optional<Audio> readWav(const std::string& path, std::string& error) {
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        error = "cannot open or parse '" + path + "' as WAV";
        return std::nullopt;
    }

    const auto channels = static_cast<std::size_t>(wav.channels);
    std::vector<float> interleaved(static_cast<std::size_t>(wav.totalPCMFrameCount) * channels);
    const auto frames = static_cast<std::size_t>(
        drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, interleaved.data()));

    Audio audio;
    audio.sampleRate = static_cast<int>(wav.sampleRate);
    drwav_uninit(&wav);

    audio.samples.resize(frames);
    for (std::size_t f = 0; f < frames; ++f) {
        float sum = 0.0f;
        for (std::size_t c = 0; c < channels; ++c) sum += interleaved[f * channels + c];
        audio.samples[f] = sum / static_cast<float>(channels);
    }
    return audio;
}

bool writeWav(const std::string& path, const Audio& audio, std::string& error) {
    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = static_cast<drwav_uint32>(audio.sampleRate);
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, path.c_str(), &format, nullptr)) {
        error = "cannot create '" + path + "'";
        return false;
    }
    const auto frames = static_cast<drwav_uint64>(audio.samples.size());
    const auto written = drwav_write_pcm_frames(&wav, frames, audio.samples.data());
    drwav_uninit(&wav);

    if (written != frames) {
        error = "short write to '" + path + "'";
        return false;
    }
    return true;
}

} // namespace ukefx
