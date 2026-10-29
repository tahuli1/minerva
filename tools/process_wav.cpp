#include "effect_chain.h"
#include "effect_set.h"
#include "offline.h"
#include "wav_io.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kBlockSize = 128;        // same block size as the live app
constexpr float kTailSeconds = 2.0f;   // room for the delay's echoes to fade out

} // namespace

// Usage: ukefx_process <in.wav> <out.wav> [effect ...]
// Runs a WAV file through the same effects as ukefx_cli, with no audio device.
// With no effects given the chain is distortion -> delay, like the CLI.
int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: ukefx_process <in.wav> <out.wav> [effect ...]\n"
                             "Known effects: %s\n",
                     std::string(ukefx::EffectSet::kKnownNames).c_str());
        return 1;
    }

    std::string error;
    auto audio = ukefx::readWav(argv[1], error);
    if (!audio) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    std::vector<std::string_view> names(argv + 3, argv + argc);
    if (names.empty()) names = {"distortion", "delay"};
    ukefx::EffectSet effects;
    ukefx::EffectChain chain;
    if (error = ukefx::buildChain(effects, names, chain); !error.empty()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    const auto inputSamples = audio->samples.size();
    ukefx::processOffline(chain, *audio, kBlockSize, kTailSeconds);

    if (!ukefx::writeWav(argv[2], *audio, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    std::printf("%s -> %s: %zu samples at %d Hz (+%.1f s tail), %zu effect(s)\n", argv[1], argv[2],
                inputSamples, audio->sampleRate, kTailSeconds, names.size());
    return 0;
}
