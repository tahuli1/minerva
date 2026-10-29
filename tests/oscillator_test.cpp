#include "dsp/oscillator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kSampleRate = 48000;

minerva::Oscillator make(minerva::Waveform waveform, float hz, bool antiAliasing = true) {
    minerva::Oscillator osc;
    osc.setSampleRate(kSampleRate);
    osc.setWaveform(waveform);
    osc.setFrequency(hz);
    osc.setAntiAliasing(antiAliasing);
    return osc;
}

// Counts how many times the signal crosses 0 going upwards: once per cycle.
int upwardCrossings(minerva::Oscillator& osc, int samples) {
    int crossings = 0;
    float previous = osc.next();
    for (int i = 1; i < samples; ++i) {
        const float value = osc.next();
        if (previous < 0.0f && value >= 0.0f) ++crossings;
        previous = value;
    }
    return crossings;
}

} // namespace

TEST(Oscillator, SineHasTheRightFrequency) {
    auto osc = make(minerva::Waveform::Sine, 440.0f);
    EXPECT_NEAR(upwardCrossings(osc, kSampleRate), 440, 1); // one second
}

TEST(Oscillator, EveryWaveformStaysWithinMinusOneToOne) {
    for (auto waveform : {minerva::Waveform::Sine, minerva::Waveform::Triangle,
                          minerva::Waveform::Saw, minerva::Waveform::Square}) {
        auto osc = make(waveform, 1000.0f);
        for (int i = 0; i < kSampleRate; ++i) {
            const float v = osc.next();
            ASSERT_LE(std::abs(v), 1.0f + 1e-5f) << "waveform " << static_cast<int>(waveform);
        }
    }
}

TEST(Oscillator, StaysAccurateAfterTenMinutes) {
    // The phase wraps every cycle, so it never grows and keeps its precision.
    auto osc = make(minerva::Waveform::Sine, 440.0f);
    for (int i = 0; i < 10 * 60 * kSampleRate; ++i) osc.next();
    EXPECT_NEAR(upwardCrossings(osc, kSampleRate), 440, 1);
}

TEST(Oscillator, PolyBlepSoftensTheSawJump) {
    // The naive saw jumps by ~2 in one sample; PolyBLEP spreads the jump over two
    // samples, which is what removes most of the aliasing.
    auto largestStep = [](bool antiAliasing) {
        auto osc = make(minerva::Waveform::Saw, 2000.0f, antiAliasing);
        float largest = 0.0f, previous = osc.next();
        for (int i = 1; i < kSampleRate / 10; ++i) {
            const float value = osc.next();
            largest = std::max(largest, std::abs(value - previous));
            previous = value;
        }
        return largest;
    };
    const float naive = largestStep(false);
    const float smoothed = largestStep(true);
    EXPECT_GT(naive, 1.9f);
    EXPECT_LT(smoothed, 0.8f * naive);
}

TEST(Oscillator, FrequencyAboveHalfTheSampleRateIsClamped) {
    auto osc = make(minerva::Waveform::Saw, 30000.0f);
    for (int i = 0; i < 1000; ++i) ASSERT_TRUE(std::isfinite(osc.next()));
}
