#include "delay.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// 1 kHz makes the math readable: 10 ms == 10 samples.
constexpr int kSampleRate = 1000;
constexpr int kBlock = 64;

void configure(ukefx::Delay& d, float timeMs, float feedback, float mix) {
    d.prepare(kSampleRate, kBlock);
    d.setTimeMs(timeMs);
    d.setFeedback(feedback);
    d.setMix(mix);
}

std::vector<float> impulse(int length) {
    std::vector<float> v(length, 0.0f);
    v[0] = 1.0f;
    return v;
}

} // namespace

TEST(Delay, ImpulseComesBackAfterTheDelayTime) {
    ukefx::Delay d;
    configure(d, 10.0f, 0.0f, 1.0f); // wet only, a single repeat
    auto in = impulse(kBlock);
    std::vector<float> out(kBlock);
    d.process(in.data(), out.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(out[i], i == 10 ? 1.0f : 0.0f) << "at " << i;
}

TEST(Delay, EachRepeatIsScaledByFeedback) {
    ukefx::Delay d;
    configure(d, 10.0f, 0.5f, 1.0f);
    auto in = impulse(kBlock);
    std::vector<float> out(kBlock);
    d.process(in.data(), out.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) {
        const float expected =
            (i > 0 && i % 10 == 0) ? static_cast<float>(std::pow(0.5, i / 10 - 1)) : 0.0f;
        EXPECT_FLOAT_EQ(out[i], expected) << "at " << i;
    }
}

TEST(Delay, MixZeroIsDryOnly) {
    ukefx::Delay d;
    configure(d, 10.0f, 0.5f, 0.0f);
    std::vector<float> in(kBlock), out(kBlock);
    for (int i = 0; i < kBlock; ++i) in[i] = std::sin(0.3f * i);
    d.process(in.data(), out.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(out[i], in[i]);
}

TEST(Delay, StateCarriesAcrossBlocks) {
    // The audio callback hands us small blocks; the echo must not care where they split.
    ukefx::Delay whole, split;
    configure(whole, 10.0f, 0.5f, 0.5f);
    configure(split, 10.0f, 0.5f, 0.5f);
    auto in = impulse(kBlock);
    std::vector<float> expected(kBlock), out(kBlock);
    whole.process(in.data(), expected.data(), kBlock);
    for (int start = 0; start < kBlock; start += 7) {
        const int n = std::min(7, kBlock - start);
        split.process(in.data() + start, out.data() + start, n);
    }
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(out[i], expected[i]) << "at " << i;
}

TEST(Delay, InPlaceMatchesOutOfPlace) {
    ukefx::Delay a, b;
    configure(a, 10.0f, 0.5f, 0.5f);
    configure(b, 10.0f, 0.5f, 0.5f);
    std::vector<float> in(kBlock), expected(kBlock);
    for (int i = 0; i < kBlock; ++i) in[i] = std::sin(0.3f * i);
    a.process(in.data(), expected.data(), kBlock);
    b.process(in.data(), in.data(), kBlock);
    for (int i = 0; i < kBlock; ++i) EXPECT_FLOAT_EQ(in[i], expected[i]) << "at " << i;
}

TEST(Delay, FeedbackIsClampedSoEchoesAlwaysDecay) {
    // Asking for feedback above 1 must not make the signal grow.
    ukefx::Delay d;
    configure(d, 10.0f, 5.0f, 1.0f);
    auto in = impulse(1000);
    std::vector<float> out(in.size());
    d.process(in.data(), out.data(), static_cast<int>(in.size()));
    float previous = 2.0f;
    for (int i = 10; i < 1000; i += 10) {
        EXPECT_LT(out[i], previous) << "at " << i;
        previous = out[i];
    }
}

TEST(Delay, TimeLongerThanTheBufferIsClamped) {
    // An absurd delay time must never read outside the buffer.
    ukefx::Delay d;
    configure(d, 1e9f, 0.0f, 1.0f);
    auto in = impulse(4096);
    std::vector<float> out(in.size());
    d.process(in.data(), out.data(), static_cast<int>(in.size()));
    for (float s : out) EXPECT_TRUE(std::isfinite(s));
}
