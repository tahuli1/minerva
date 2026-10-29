#include "effects/distortion/distortion.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBlock = 8;

void configure(minerva::Distortion& d, float drive, float level) {
    d.setDrive(drive);
    d.setLevel(level);
    d.prepare(kSampleRate, kBlock);
}

} // namespace

TEST(Distortion, SilenceStaysSilent) {
    minerva::Distortion d;
    configure(d, 0.8f, 1.0f);
    std::array<float, kBlock> in{}, out{};
    out.fill(1.0f);
    d.process(in.data(), out.data(), kBlock);
    for (float s : out) EXPECT_EQ(s, 0.0f);
}

TEST(Distortion, OutputStaysBoundedForAnyDrive) {
    // Any input in [-1, 1], any drive (out-of-range drives are clamped): the output stays
    // below 1.08 * level. Fails if drive 1.0 is allowed (DaisySP's ~3x jump).
    const float level = 0.5f;
    float loudest = 0.0f;
    for (int step = 0; step <= 20; ++step) { // whole steps: 0.05f added 20 times overshoots 1.0f
        minerva::Distortion d;
        configure(d, static_cast<float>(step) / 20.0f, level);
        std::vector<float> in(401), out(401);
        for (int k = 0; k <= 400; ++k) in[k] = -1.0f + static_cast<float>(k) / 200.0f;
        d.process(in.data(), out.data(), 401);
        for (float s : out) loudest = std::max(loudest, std::abs(s));
    }
    EXPECT_LE(loudest, 1.08f * level);
}

TEST(Distortion, IsOddSymmetric) {
    // f(-x) == -f(x): no DC offset.
    minerva::Distortion d;
    configure(d, 0.6f, 1.0f);
    std::array<float, kBlock> pos{0.01f, 0.05f, 0.1f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
    std::array<float, kBlock> neg{}, outPos{}, outNeg{};
    for (int i = 0; i < kBlock; ++i) neg[i] = -pos[i];
    d.process(pos.data(), outPos.data(), kBlock);
    d.process(neg.data(), outNeg.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(outNeg[i], -outPos[i]);
}

TEST(Distortion, SmallSignalsAreAmplifiedLinearly) {
    // Near zero a soft clipper is linear: doubling a tiny input doubles the output.
    minerva::Distortion d;
    configure(d, 0.4f, 1.0f);
    std::array<float, 2> in{1e-4f, 2e-4f}, out{};
    d.process(in.data(), out.data(), 2);
    EXPECT_GT(out[0], 1e-4f);
    EXPECT_NEAR(out[1] / out[0], 2.0f, 0.01f);
}

TEST(Distortion, InPlaceMatchesOutOfPlace) {
    minerva::Distortion a, b;
    configure(a, 0.7f, 0.8f);
    configure(b, 0.7f, 0.8f);
    std::array<float, kBlock> in{-1.0f, -0.5f, -0.1f, 0.0f, 0.1f, 0.3f, 0.7f, 1.0f};
    std::array<float, kBlock> expected{};
    a.process(in.data(), expected.data(), kBlock);
    b.process(in.data(), in.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(in[i], expected[i]);
}

TEST(Distortion, DriveChangeGlidesInsteadOfJumping) {
    // Drive 0.1 -> 0.9 on a steady input: small steps over 20 ms (960 samples), then
    // exactly the output of a distortion set to 0.9 from the start.
    const float input = 0.05f;
    minerva::Distortion d, settled;
    configure(d, 0.1f, 1.0f);
    configure(settled, 0.9f, 1.0f);
    std::vector<float> in(2000, input), out(2000);
    d.process(in.data(), out.data(), 100);
    d.setDrive(0.9f);
    d.process(in.data() + 100, out.data() + 100, 1900);
    float target = 0.0f;
    settled.process(&input, &target, 1);

    const float change = std::abs(target - out[99]);
    ASSERT_GT(change, 0.5f);
    for (int i = 100; i < 2000; ++i)
        EXPECT_LT(std::abs(out[i] - out[i - 1]), change / 50.0f) << "jump at " << i;
    EXPECT_FLOAT_EQ(out[100 + 960], target);
}

TEST(Distortion, WhenOffTheInputPassesThroughUnchanged) {
    minerva::Distortion d;
    d.setEnabled(false);
    configure(d, 0.9f, 0.3f);
    std::array<float, kBlock> in{-1.0f, -0.5f, -0.1f, 0.0f, 0.1f, 0.3f, 0.7f, 1.0f}, out{};
    d.process(in.data(), out.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_EQ(out[i], in[i]);
}

TEST(Distortion, SwitchingOnCrossfadesInsteadOfJumping) {
    // Off -> on on a steady input: small steps over 20 ms (960 samples), then exactly
    // the distorted output.
    const float input = 0.5f;
    minerva::Distortion d, on;
    d.setEnabled(false);
    configure(d, 0.9f, 1.0f);
    configure(on, 0.9f, 1.0f);
    std::vector<float> in(2000, input), out(2000);
    d.process(in.data(), out.data(), 100);
    d.setEnabled(true);
    d.process(in.data() + 100, out.data() + 100, 1900);
    float target = 0.0f;
    on.process(&input, &target, 1);

    const float change = std::abs(target - out[99]);
    ASSERT_GT(change, 0.1f);
    for (int i = 100; i < 2000; ++i)
        EXPECT_LT(std::abs(out[i] - out[i - 1]), change / 50.0f) << "jump at " << i;
    EXPECT_FLOAT_EQ(out[100 + 960], target);
}
