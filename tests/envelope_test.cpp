#include "dsp/envelope.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

// 1 kHz: 1 ms is exactly 1 sample, so times read as sample counts.
constexpr int kSampleRate = 1000;

minerva::Envelope make(float attack, float decay, float sustain, float release) {
    minerva::Envelope env;
    env.setSampleRate(kSampleRate);
    env.setParams({attack, decay, sustain, release});
    return env;
}

float advance(minerva::Envelope& env, int samples) {
    float level = 0.0f;
    for (int i = 0; i < samples; ++i) level = env.next();
    return level;
}

} // namespace

TEST(Envelope, StartsSilentAndIdle) {
    auto env = make(0.01f, 0.02f, 0.5f, 0.05f);
    EXPECT_FALSE(env.isActive());
    EXPECT_EQ(advance(env, 10), 0.0f);
}

TEST(Envelope, FollowsAttackDecaySustainRelease) {
    auto env = make(0.01f, 0.02f, 0.5f, 0.05f); // A 10 samples, D 20, S 0.5, R 50
    env.noteOn();
    EXPECT_EQ(advance(env, 10), 1.0f);  // top of the attack
    EXPECT_EQ(advance(env, 20), 0.5f);  // decayed to the sustain level
    EXPECT_EQ(advance(env, 500), 0.5f); // held there as long as the key is down
    EXPECT_TRUE(env.isActive());

    env.noteOff();
    EXPECT_GT(advance(env, 25), 0.0f);  // halfway through the release
    EXPECT_EQ(advance(env, 25), 0.0f);  // silent after exactly R
    EXPECT_FALSE(env.isActive());
}

TEST(Envelope, NoteOnDuringReleaseContinuesFromTheCurrentLevel) {
    auto env = make(0.01f, 0.0f, 1.0f, 0.1f);
    env.noteOn();
    advance(env, 20);
    env.noteOff();
    const float fading = advance(env, 50); // halfway down
    env.noteOn();
    const float next = env.next();
    EXPECT_GT(next, fading);                     // rising again...
    EXPECT_LT(next - fading, 0.11f);             // ...by one attack step, not a jump
}

TEST(Envelope, ZeroTimesJumpStraightToTheirTarget) {
    auto env = make(0.0f, 0.0f, 0.7f, 0.0f);
    env.noteOn();
    EXPECT_FLOAT_EQ(env.next(), 0.7f);
    env.noteOff();
    EXPECT_EQ(env.next(), 0.0f);
    EXPECT_FALSE(env.isActive());
}

TEST(Envelope, ZeroSustainGoesSilentWhileTheKeyIsStillDown) {
    // A "pluck": it fades out even though the key is held, but the note is still on.
    auto env = make(0.002f, 0.3f, 0.0f, 0.1f);
    env.noteOn();
    EXPECT_EQ(advance(env, 400), 0.0f);
    EXPECT_TRUE(env.isActive());
}
