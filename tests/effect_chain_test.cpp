#include "effects/effect_chain.h"

#include <gtest/gtest.h>

#include <array>

namespace {

constexpr int kBlock = 4;

// Tiny test-only effects whose results depend on the order they run in.
struct Doubler : ukefx::Effect {
    void prepare(int, int) override {}
    void process(const float* in, float* out, int n) override {
        for (int i = 0; i < n; ++i) out[i] = 2.0f * in[i];
    }
};

struct AddOne : ukefx::Effect {
    void prepare(int, int) override {}
    void process(const float* in, float* out, int n) override {
        for (int i = 0; i < n; ++i) out[i] = in[i] + 1.0f;
    }
};

struct PrepareRecorder : ukefx::Effect {
    int sampleRate = 0;
    int maxBlockSize = 0;
    void prepare(int sr, int maxBlock) override {
        sampleRate = sr;
        maxBlockSize = maxBlock;
    }
    void process(const float* in, float* out, int n) override {
        for (int i = 0; i < n; ++i) out[i] = in[i];
    }
};

} // namespace

TEST(EffectChain, EmptyChainPassesInputThrough) {
    ukefx::EffectChain chain;
    chain.prepare(48000, kBlock);
    std::array<float, kBlock> in{0.1f, -0.2f, 0.3f, -0.4f}, out{};
    chain.process(in.data(), out.data(), kBlock);
    EXPECT_EQ(out, in);
}

TEST(EffectChain, RunsEffectsInTheOrderTheyWereAdded) {
    Doubler doubler;
    AddOne addOne;
    std::array<float, kBlock> in{1.0f, 2.0f, 3.0f, 4.0f}, out{};

    ukefx::EffectChain doubleThenAdd;
    doubleThenAdd.add(doubler);
    doubleThenAdd.add(addOne);
    doubleThenAdd.process(in.data(), out.data(), kBlock);
    EXPECT_EQ(out, (std::array<float, kBlock>{3.0f, 5.0f, 7.0f, 9.0f})); // 2x + 1

    ukefx::EffectChain addThenDouble;
    addThenDouble.add(addOne);
    addThenDouble.add(doubler);
    addThenDouble.process(in.data(), out.data(), kBlock);
    EXPECT_EQ(out, (std::array<float, kBlock>{4.0f, 6.0f, 8.0f, 10.0f})); // 2(x + 1)
}

TEST(EffectChain, DoesNotModifyTheInput) {
    Doubler doubler;
    AddOne addOne;
    ukefx::EffectChain chain;
    chain.add(doubler);
    chain.add(addOne);
    const std::array<float, kBlock> original{1.0f, 2.0f, 3.0f, 4.0f};
    std::array<float, kBlock> in = original, out{};
    chain.process(in.data(), out.data(), kBlock);
    EXPECT_EQ(in, original);
}

TEST(EffectChain, PreparesEveryEffect) {
    PrepareRecorder a, b;
    ukefx::EffectChain chain;
    chain.add(a);
    chain.add(b);
    chain.prepare(44100, 256);
    EXPECT_EQ(a.sampleRate, 44100);
    EXPECT_EQ(a.maxBlockSize, 256);
    EXPECT_EQ(b.sampleRate, 44100);
    EXPECT_EQ(b.maxBlockSize, 256);
}

TEST(EffectChain, RejectsEffectsBeyondCapacity) {
    AddOne addOne;
    ukefx::EffectChain chain;
    for (int i = 0; i < ukefx::EffectChain::kMaxEffects; ++i) EXPECT_TRUE(chain.add(addOne));
    EXPECT_FALSE(chain.add(addOne));
    EXPECT_EQ(chain.size(), ukefx::EffectChain::kMaxEffects);
}
