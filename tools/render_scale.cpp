#include "synth/voice.h"
#include "wav_io.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBlockSize = 128; // render in the same blocks as the live app

bool parseWaveform(const char* name, minerva::Waveform& waveform) {
    if (std::strcmp(name, "sine") == 0) waveform = minerva::Waveform::Sine;
    else if (std::strcmp(name, "triangle") == 0) waveform = minerva::Waveform::Triangle;
    else if (std::strcmp(name, "saw") == 0) waveform = minerva::Waveform::Saw;
    else if (std::strcmp(name, "square") == 0) waveform = minerva::Waveform::Square;
    else return false;
    return true;
}

void renderSeconds(minerva::Voice& voice, minerva::Audio& audio, float seconds) {
    const auto total = static_cast<int>(seconds * kSampleRate);
    for (int done = 0; done < total; done += kBlockSize) {
        const int n = std::min(kBlockSize, total - done);
        const auto start = audio.samples.size();
        audio.samples.resize(start + static_cast<std::size_t>(n), 0.0f);
        voice.render(audio.samples.data() + start, n);
    }
}

} // namespace

// Usage: minerva_scale <out.wav> <sine|triangle|saw|square> [octave] [--no-aa]
// Renders a C major scale (8 notes, half a second each) with one voice, so you can
// hear the oscillator and envelope offline. --no-aa turns PolyBLEP off for comparison;
// the difference is clearest in a high octave, e.g. 6.
int main(int argc, char** argv) {
    minerva::Waveform waveform{};
    if (argc < 3 || !parseWaveform(argv[2], waveform)) {
        std::fprintf(stderr, "Usage: minerva_scale <out.wav> <sine|triangle|saw|square> [octave] [--no-aa]\n");
        return 1;
    }
    int octave = 4;
    bool antiAliasing = true;
    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--no-aa") == 0) antiAliasing = false;
        else octave = std::atoi(argv[i]);
    }

    minerva::Voice voice;
    voice.prepare(kSampleRate);
    voice.setWaveform(waveform);
    voice.setEnvelope({0.01f, 0.1f, 0.7f, 0.1f});
    voice.setAntiAliasing(antiAliasing);

    minerva::Audio audio;
    audio.sampleRate = kSampleRate;
    const int cMajor[] = {0, 2, 4, 5, 7, 9, 11, 12}; // semitones above C
    for (int step : cMajor) {
        voice.start(12 * (octave + 1) + step, 0.5f); // note 60 is C4
        renderSeconds(voice, audio, 0.35f);
        voice.release();
        renderSeconds(voice, audio, 0.15f);
    }

    std::string error;
    if (!minerva::writeWav(argv[1], audio, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    std::printf("%s: C major scale, octave %d, %s%s\n", argv[1], octave, argv[2],
                antiAliasing ? "" : " (no anti-aliasing)");
    return 0;
}
