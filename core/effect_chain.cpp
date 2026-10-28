#include "effect_chain.h"

#include <algorithm>

namespace ukefx {

bool EffectChain::add(Effect& effect) {
    if (count_ == kMaxEffects) return false;
    effects_[count_++] = &effect;
    return true;
}

void EffectChain::prepare(int sampleRate, int maxBlockSize) {
    for (int i = 0; i < count_; ++i) effects_[i]->prepare(sampleRate, maxBlockSize);
}

void EffectChain::process(const float* in, float* out, int n) {
    if (count_ == 0) {
        std::copy_n(in, n, out);
        return;
    }
    // The first effect reads the input; every later one works in place on `out`.
    // No intermediate buffers are needed, because every Effect supports in == out.
    effects_[0]->process(in, out, n);
    for (int i = 1; i < count_; ++i) effects_[i]->process(out, out, n);
}

} // namespace ukefx
