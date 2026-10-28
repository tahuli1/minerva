#include "effects/distortion.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBlock = 8;

void configure(ukefx::Distortion& d, float drive, float level) {
    d.setDrive(drive);
    d.setLevel(level);
    d.prepare(kSampleRate, kBlock);
}

} // namespace

TEST(Distortion, SilenceStaysSilent) {
    ukefx::Distortion d;
    configure(d, 20.0f, 1.0f);
    std::array<float, kBlock> in{}, out{};
    out.fill(1.0f);
    d.process(in.data(), out.data(), kBlock);
    for (float s : out) EXPECT_EQ(s, 0.0f);
}

TEST(Distortion, OutputNeverExceedsLevel) {
    // Even absurd input must stay within +/-level: this is what protects the ears.
    const float level = 0.5f;
    ukefx::Distortion d;
    configure(d, 50.0f, level);
    std::array<float, kBlock> in{-100.0f, -1.0f, -0.1f, 0.0f, 0.1f, 1.0f, 100.0f, 1e6f};
    std::array<float, kBlock> out{};
    d.process(in.data(), out.data(), kBlock);
    for (float s : out) EXPECT_LE(std::abs(s), level);
}

TEST(Distortion, IsOddSymmetric) {
    // f(-x) == -f(x): no DC offset and only odd harmonics, like a symmetric tube stage.
    ukefx::Distortion d;
    configure(d, 8.0f, 1.0f);
    std::array<float, kBlock> pos{0.01f, 0.05f, 0.1f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
    std::array<float, kBlock> neg{}, outPos{}, outNeg{};
    for (int i = 0; i < kBlock; ++i) neg[i] = -pos[i];
    d.process(pos.data(), outPos.data(), kBlock);
    d.process(neg.data(), outNeg.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(outNeg[i], -outPos[i]);
}

TEST(Distortion, SmallSignalsAreAmplifiedLinearly) {
    // tanh(u) ~= u for small u, so quiet playing just gets louder by `drive`.
    const float drive = 4.0f;
    ukefx::Distortion d;
    configure(d, drive, 1.0f);
    std::array<float, kBlock> in{}, out{};
    in.fill(1e-3f);
    d.process(in.data(), out.data(), kBlock);
    for (float s : out) EXPECT_NEAR(s, drive * 1e-3f, 1e-6f);
}

TEST(Distortion, InPlaceMatchesOutOfPlace) {
    // The effect chain will process in place, so in == out must work.
    ukefx::Distortion d;
    configure(d, 10.0f, 0.8f);
    std::array<float, kBlock> in{-1.0f, -0.5f, -0.1f, 0.0f, 0.1f, 0.3f, 0.7f, 1.0f};
    std::array<float, kBlock> expected{};
    d.process(in.data(), expected.data(), kBlock);
    d.process(in.data(), in.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(in[i], expected[i]);
}

TEST(Distortion, DriveChangeGlidesInsteadOfJumping) {
    // Without smoothing, jumping from drive 1 to 20 on a steady 0.05 input would make
    // the output leap from ~0.05 to ~0.76 in one sample: an audible click.
    ukefx::Distortion d;
    configure(d, 1.0f, 1.0f);
    std::vector<float> in(2000, 0.05f), out(2000);
    d.process(in.data(), out.data(), 100);
    d.setDrive(20.0f);
    d.process(in.data() + 100, out.data() + 100, 1900);

    for (int i = 100; i < 2000; ++i)
        EXPECT_LT(std::abs(out[i] - out[i - 1]), 0.002f) << "jump at " << i;
    // 20 ms at 48 kHz is 960 samples; after that the new drive is fully applied.
    EXPECT_FLOAT_EQ(out[100 + 960], std::tanh(20.0f * 0.05f));
}
