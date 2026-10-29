#include "effect_set.h"
#include "offline.h"
#include "wav_io.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string tempPath(const char* name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

} // namespace

TEST(Offline, ImpulseThroughTheDelayFromWavToWav) {
    // End to end, through real files: write an impulse WAV, run it through the delay
    // in 128-sample blocks (like the live callback), write the result, read it back.
    // At 48 kHz the default 350 ms delay is 16800 samples: not a multiple of 128, so
    // every echo crosses a block boundary.
    const std::string inPath = tempPath("ukefx_offline_in.wav");
    const std::string outPath = tempPath("ukefx_offline_out.wav");
    std::string error;

    ukefx::Audio impulse;
    impulse.sampleRate = 48000;
    impulse.samples.assign(100, 0.0f);
    impulse.samples[0] = 1.0f;
    ASSERT_TRUE(ukefx::writeWav(inPath, impulse, error)) << error;

    auto audio = ukefx::readWav(inPath, error);
    ASSERT_TRUE(audio) << error;
    ukefx::EffectSet effects; // delay defaults: 350 ms, feedback 0.4, mix 0.35
    ukefx::EffectChain chain;
    const std::vector<std::string_view> names{"delay"};
    ASSERT_EQ(ukefx::buildChain(effects, names, chain), "");
    ukefx::processOffline(chain, *audio, 128, 1.0f);
    ASSERT_TRUE(ukefx::writeWav(outPath, *audio, error)) << error;

    const auto result = ukefx::readWav(outPath, error);
    ASSERT_TRUE(result) << error;
    std::filesystem::remove(inPath);
    std::filesystem::remove(outPath);

    const auto& out = result->samples;
    ASSERT_EQ(out.size(), 100u + 48000u); // input plus a 1 s tail
    EXPECT_NEAR(out[0], 0.65f, 1e-6f);        // dry: 1 - mix
    EXPECT_NEAR(out[16800], 0.35f, 1e-6f);    // first echo: mix
    EXPECT_NEAR(out[33600], 0.14f, 1e-6f);    // second echo: mix * feedback
    EXPECT_EQ(out[1], 0.0f);
    EXPECT_EQ(out[16799], 0.0f);
    EXPECT_EQ(out[16801], 0.0f);
}
