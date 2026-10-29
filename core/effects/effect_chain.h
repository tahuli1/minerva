#pragma once

#include "effects/effect.h"

#include <array>

namespace ukefx {

// Runs effects one after another. It is itself an Effect, so the backend
// doesn't care whether it runs one effect or a whole chain.
// Does not own the effects; they must outlive the chain.
class EffectChain : public Effect {
public:
    static constexpr int kMaxEffects = 8;

    // Not RT, and not while audio runs: call before prepare(). Changing the chain
    // live needs a lock-free swap, which comes with the GUI (Milestone 6).
    // Returns false if the chain is full.
    bool add(Effect& effect);

    void prepare(int sampleRate, int maxBlockSize) override;
    void process(const float* in, float* out, int n) override;

    int size() const { return count_; }

private:
    // Fixed capacity instead of std::vector: nothing about the chain can allocate.
    std::array<Effect*, kMaxEffects> effects_{};
    int count_ = 0;
};

} // namespace ukefx
