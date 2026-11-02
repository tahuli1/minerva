#include "rt/timing_stats.h"

#include <gtest/gtest.h>

namespace {

// 1 tick = 1 microsecond, so the numbers below read directly as microseconds.
constexpr double kTicksPerSecond = 1'000'000.0;
constexpr int kSampleRate = 48000;
constexpr std::uint32_t kFrames = 128; // 2.6667 ms

minerva::CallbackTiming timing(std::int64_t startUs, std::int64_t durationUs) {
    return {startUs, durationUs, kFrames};
}

} // namespace

TEST(TimingStats, EmptyHasNoCallbacks) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    const minerva::TimingSummary s = stats.summary();
    EXPECT_EQ(s.callbacks, 0);
    EXPECT_EQ(s.maxMs, 0.0);
    EXPECT_EQ(s.maxGapMs, 0.0);
}

TEST(TimingStats, AverageAndMaximum) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    stats.add(timing(2667, 300));
    stats.add(timing(5333, 200));
    const minerva::TimingSummary s = stats.summary();
    EXPECT_EQ(s.callbacks, 3);
    EXPECT_DOUBLE_EQ(s.avgMs, 0.2);
    EXPECT_DOUBLE_EQ(s.maxMs, 0.3);
}

TEST(TimingStats, DeadlineIsOneBlockPeriod) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    EXPECT_NEAR(stats.summary().deadlineMs, 2.6667, 0.0001);
}

TEST(TimingStats, PercentileIgnoresTheRareOutlierButMaxShowsIt) {
    // 999 fast callbacks and one slow one: the 99.9th percentile is still fast,
    // the maximum is the slow one.
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    for (int i = 0; i < 999; ++i) stats.add(timing(i * 2667, 100));
    stats.add(timing(999 * 2667, 2000));
    const minerva::TimingSummary s = stats.summary();
    EXPECT_DOUBLE_EQ(s.p999Ms, 0.1);
    EXPECT_DOUBLE_EQ(s.maxMs, 2.0);
}

TEST(TimingStats, LongestGapBetweenCallbackStarts) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    stats.add(timing(2667, 100));
    stats.add(timing(12667, 100)); // held back for 10 ms
    stats.add(timing(15334, 100));
    EXPECT_DOUBLE_EQ(stats.summary().maxGapMs, 10.0);
}

TEST(TimingStats, ResetStartsAFreshPeriodButKeepsTheGapAcrossIt) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 500));
    stats.reset();
    stats.add(timing(8000, 100)); // 8 ms after the callback before the reset
    const minerva::TimingSummary s = stats.summary();
    EXPECT_EQ(s.callbacks, 1);
    EXPECT_DOUBLE_EQ(s.maxMs, 0.1);
    EXPECT_DOUBLE_EQ(s.maxGapMs, 8.0);
}

TEST(TimingStats, AGapOverOneAndAHalfBlocksIsLate) {
    // One block is 2666.7 us, so the threshold is 4000 us.
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    stats.add(timing(3900, 100));  // gap 3900: jitter, not late
    stats.add(timing(8001, 100));  // gap 4101: late
    stats.add(timing(10668, 100)); // gap 2667: on time
    EXPECT_EQ(stats.summary().lateCallbacks, 1);
}

TEST(TimingStats, OneLongGapCountsOnce) {
    // However long the stall, it is one late callback, not one per missed block.
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    stats.add(timing(50000, 100));
    EXPECT_EQ(stats.summary().lateCallbacks, 1);
}

TEST(TimingStats, LatenessFollowsTheBlockSize) {
    // With 256-frame blocks (5333 us) a 4500 us gap is on time.
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add({0, 100, 256});
    stats.add({4500, 100, 256});
    EXPECT_EQ(stats.summary().lateCallbacks, 0);
}

TEST(TimingStats, ResetClearsLateCallbacks) {
    minerva::TimingStats stats(kTicksPerSecond, kSampleRate);
    stats.add(timing(0, 100));
    stats.add(timing(10000, 100));
    stats.reset();
    stats.add(timing(12667, 100));
    EXPECT_EQ(stats.summary().lateCallbacks, 0);
}
