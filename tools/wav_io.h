#pragma once

#include <optional>
#include <string>
#include <vector>

namespace ukefx {

// Mono audio held in memory. Offline only: unlike the audio thread, this code
// allocates freely.
struct Audio {
    int sampleRate = 48000;
    std::vector<float> samples;
};

// Reads a PCM (16/24/32-bit) or float WAV. Multi-channel files are averaged down
// to mono, because the effects are mono (a ukulele is one source).
// Returns nullopt and sets `error` on failure.
std::optional<Audio> readWav(const std::string& path, std::string& error);

// Writes a mono 32-bit float WAV. Float keeps every sample exact (no rounding to
// 16-bit), so a test can write a file, read it back and compare values exactly.
bool writeWav(const std::string& path, const Audio& audio, std::string& error);

} // namespace ukefx
