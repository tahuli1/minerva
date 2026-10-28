#include "denormals.h"
#include "effects/delay.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

// A 1-sample echo with high feedback: the tail decays by 0.9 every sample and
// drops through the denormal range (~1e-38 down to ~1e-45) within ~1000 samples.
int countDenormalsInDelayTail() {
    ukefx::Delay delay;
    delay.prepare(1000, 2000);
    delay.setTimeMs(1.0f);
    delay.setFeedback(0.9f);
    delay.setMix(1.0f);
    std::vector<float> in(2000, 0.0f), out(2000);
    in[0] = 1.0f;
    delay.process(in.data(), out.data(), static_cast<int>(in.size()));

    int denormals = 0;
    for (float s : out)
        if (std::fpclassify(s) == FP_SUBNORMAL) ++denormals;
    return denormals;
}

} // namespace

TEST(Denormals, DelayTailProducesThemByDefault) {
    // Documents the problem: without FTZ/DAZ a decaying echo really does hit denormals.
    EXPECT_GT(countDenormalsInDelayTail(), 0);
}

TEST(Denormals, ScopedNoDenormalsFlushesThemToZero) {
    ukefx::ScopedNoDenormals noDenormals;
    EXPECT_EQ(countDenormalsInDelayTail(), 0);
}
