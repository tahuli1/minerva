#include "dsp/smoothed_value.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

// 1 kHz and 10 ms: a ramp is exactly 10 samples.
constexpr int kSampleRate = 1000;
constexpr float kRampSeconds = 0.01f;
constexpr int kRampSamples = 10;

} // namespace

TEST(SmoothedValue, StartsAtTheInitialValue) {
    minerva::SmoothedValue v;
    v.reset(kSampleRate, kRampSeconds, 0.5f);
    for (int i = 0; i < 20; ++i) EXPECT_EQ(v.next(), 0.5f);
}

TEST(SmoothedValue, ReachesTheTargetExactlyAfterTheRamp) {
    minerva::SmoothedValue v;
    v.reset(kSampleRate, kRampSeconds, 0.0f);
    v.setTarget(1.0f);
    for (int i = 0; i < kRampSamples - 1; ++i) EXPECT_LT(v.next(), 1.0f);
    EXPECT_EQ(v.next(), 1.0f);
    for (int i = 0; i < 20; ++i) EXPECT_EQ(v.next(), 1.0f);
}

TEST(SmoothedValue, MovesInEqualSteps) {
    minerva::SmoothedValue v;
    v.reset(kSampleRate, kRampSeconds, 0.0f);
    v.setTarget(1.0f);
    float previous = v.current();
    for (int i = 0; i < kRampSamples; ++i) {
        const float value = v.next();
        EXPECT_NEAR(value - previous, 0.1f, 1e-6f) << "step " << i;
        previous = value;
    }
}

TEST(SmoothedValue, RepeatingTheSameTargetDoesNotRestartTheRamp) {
    // The audio thread calls setTarget() every block, usually with an unchanged value.
    minerva::SmoothedValue v;
    v.reset(kSampleRate, kRampSeconds, 0.0f);
    v.setTarget(1.0f);
    for (int i = 0; i < 5; ++i) v.next();
    v.setTarget(1.0f);
    for (int i = 0; i < 4; ++i) v.next();
    EXPECT_EQ(v.next(), 1.0f); // still arrives after 10 samples in total
}

TEST(SmoothedValue, NewTargetMidRampContinuesFromTheCurrentValue) {
    minerva::SmoothedValue v;
    v.reset(kSampleRate, kRampSeconds, 0.0f);
    v.setTarget(1.0f);
    for (int i = 0; i < 5; ++i) v.next(); // now at 0.5
    const float before = v.current();
    v.setTarget(0.0f);
    const float after = v.next();
    EXPECT_LT(std::abs(after - before), 0.1f); // one small step, no jump
    EXPECT_LT(after, before);                  // already heading to the new target
}

TEST(SmoothedValue, ZeroRampTimeJumpsImmediately) {
    minerva::SmoothedValue v;
    v.reset(kSampleRate, 0.0f, 0.0f);
    v.setTarget(0.7f);
    EXPECT_EQ(v.next(), 0.7f);
}
