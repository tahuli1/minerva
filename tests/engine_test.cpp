#include "engine/engine.h"

#include "allocation_counter.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBlock = 128;

// One audio block; returns its loudest sample.
float block(minerva::Engine& engine) {
    std::vector<float> out(kBlock);
    engine.process(nullptr, out.data(), kBlock);
    float loudest = 0.0f;
    for (float s : out) loudest = std::max(loudest, std::abs(s));
    return loudest;
}

void prepare(minerva::Engine& engine) {
    engine.synth().setEnvelope({0.005f, 0.05f, 0.8f, 0.1f});
    engine.delay().setMix(0.0f); // dry, so silence means silence
    engine.prepare(kSampleRate, kBlock);
}

} // namespace

TEST(Engine, IsSilentWithoutNotes) {
    minerva::Engine engine;
    prepare(engine);
    EXPECT_EQ(block(engine), 0.0f);
}

TEST(Engine, ANoteOnThroughTheQueueMakesSound) {
    minerva::Engine engine;
    prepare(engine);
    ASSERT_TRUE(engine.noteOn(69, 1.0f));
    block(engine); // the attack
    EXPECT_GT(block(engine), 0.01f);
    EXPECT_TRUE(engine.synth().isSounding(69));
}

TEST(Engine, ANoteOffThroughTheQueueEndsTheNote) {
    minerva::Engine engine;
    prepare(engine);
    engine.noteOn(69, 1.0f);
    block(engine);
    engine.noteOff(69);
    for (int i = 0; i < 50; ++i) block(engine); // ~130 ms, longer than the 100 ms release
    EXPECT_FALSE(engine.synth().isSounding(69));
    EXPECT_EQ(block(engine), 0.0f);
}

TEST(Engine, CommandsAreAppliedInOrder) {
    // On then off before the next block: the note starts and is released in that block.
    // Off then on: the note keeps playing. Order is what makes the two different.
    minerva::Engine a, b;
    prepare(a);
    prepare(b);
    a.noteOn(60, 1.0f);
    a.noteOff(60);
    b.noteOff(60);
    b.noteOn(60, 1.0f);
    for (int i = 0; i < 50; ++i) {
        block(a);
        block(b);
    }
    EXPECT_FALSE(a.synth().isSounding(60));
    EXPECT_TRUE(b.synth().isSounding(60));
}

TEST(Engine, AFullQueueRefusesInsteadOfWaiting) {
    minerva::Engine engine;
    prepare(engine);
    int accepted = 0;
    while (engine.noteOn(60, 1.0f) && accepted < 10000) ++accepted;
    EXPECT_GE(accepted, static_cast<int>(minerva::Engine::kQueueCapacity) - 1);
    EXPECT_LT(accepted, 10000); // it did refuse, and returned rather than blocking
}

TEST(Engine, WorkPerBlockIsBounded) {
    // At most kMaxCommandsPerBlock commands are handled per block; the rest wait.
    minerva::Engine engine;
    prepare(engine);
    for (int i = 0; i < 100; ++i) engine.noteOn(40 + i % 40, 1.0f);
    block(engine);
    EXPECT_EQ(engine.pendingCommands(), 100u - minerva::Engine::kMaxCommandsPerBlock);
    block(engine);
    EXPECT_EQ(engine.pendingCommands(), 0u);
}

TEST(Engine, AWaveformChangeThroughTheQueueChangesTheSound) {
    minerva::Engine sine, square;
    prepare(sine);
    prepare(square);
    sine.setWaveform(minerva::Waveform::Sine);
    square.setWaveform(minerva::Waveform::Square);
    sine.noteOn(69, 1.0f);
    square.noteOn(69, 1.0f);
    std::vector<float> a(kBlock), b(kBlock);
    for (int i = 0; i < 4; ++i) { // past the attack
        sine.process(nullptr, a.data(), kBlock);
        square.process(nullptr, b.data(), kBlock);
    }
    float difference = 0.0f;
    for (int i = 0; i < kBlock; ++i) difference = std::max(difference, std::abs(a[i] - b[i]));
    EXPECT_GT(difference, 0.01f);
}

TEST(Engine, ProcessNeverAllocatesOrFrees) {
    // The rule the whole audio path depends on, checked rather than promised.
    // Twenty seconds of busy playing: chords past the 16 voices (so voices are
    // stolen), releases, waveform changes, the distortion switched on and off and
    // the delay's parameters moving. The counter is armed only around process(),
    // the audio thread's part; pushing commands is the control thread's.
    minerva::Engine engine;
    engine.distortion().setEnabled(true);
    engine.prepare(kSampleRate, kBlock); // allocating here is allowed

    std::vector<float> out(kBlock); // the backend's buffer, allocated before audio
    int allocations = 0;
    int frees = 0;
    for (int b = 0; b < kSampleRate / kBlock * 20; ++b) {
        if (b % 10 == 0) engine.noteOn(36 + (b / 10) % 48, 0.8f);
        if (b % 10 == 5 && b > 200) engine.noteOff(36 + (b / 10 - 20) % 48);
        if (b % 500 == 0) engine.setWaveform(static_cast<minerva::Waveform>((b / 500) % 4));
        if (b % 700 == 0) engine.distortion().setEnabled((b / 700) % 2 == 0);
        if (b % 300 == 0) {
            engine.delay().setFeedback(0.2f + 0.1f * static_cast<float>((b / 300) % 7));
            engine.delay().setMix(0.1f * static_cast<float>((b / 300) % 10));
            engine.delay().setTimeMs(100.0f + 50.0f * static_cast<float>((b / 300) % 10));
        }
        test::CountAllocations count;
        engine.process(nullptr, out.data(), kBlock);
        allocations += count.allocations();
        frees += count.frees();
    }
    EXPECT_GT(engine.synth().activeVoices(), 0); // it really played
    EXPECT_EQ(allocations, 0);
    EXPECT_EQ(frees, 0);
}
