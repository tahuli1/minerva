#include "effects/distortion.h"

#include <cmath>

namespace ukefx {

void Distortion::prepare(int /*sampleRate*/, int /*maxBlockSize*/) {
    // Stateless: nothing to allocate or precompute.
}

void Distortion::process(const float* in, float* out, int n) {
    // Read each parameter once per block, so the whole block uses one consistent value.
    const float drive = drive_.load(std::memory_order_relaxed);
    const float level = level_.load(std::memory_order_relaxed);

    for (int i = 0; i < n; ++i)
        out[i] = level * std::tanh(drive * in[i]);
}

} // namespace ukefx
