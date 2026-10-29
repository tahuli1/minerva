#include "effect_set.h"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace {

std::string build(ukefx::EffectSet& effects, ukefx::EffectChain& chain,
                  std::vector<std::string_view> names) {
    return ukefx::buildChain(effects, names, chain);
}

} // namespace

TEST(EffectSet, FindsEffectsByName) {
    ukefx::EffectSet effects;
    EXPECT_EQ(effects.find("distortion"), &effects.distortion);
    EXPECT_EQ(effects.find("delay"), &effects.delay);
    EXPECT_EQ(effects.find("reverb"), nullptr);
}

TEST(EffectSet, BuildsTheChainInTheGivenOrder) {
    ukefx::EffectSet effects;
    ukefx::EffectChain chain;
    EXPECT_EQ(build(effects, chain, {"delay", "distortion"}), "");
    EXPECT_EQ(chain.size(), 2);
}

TEST(EffectSet, RejectsAnUnknownName) {
    ukefx::EffectSet effects;
    ukefx::EffectChain chain;
    const std::string error = build(effects, chain, {"delay", "flanger"});
    EXPECT_NE(error.find("unknown effect 'flanger'"), std::string::npos) << error;
}

TEST(EffectSet, RejectsTheSameEffectTwice) {
    ukefx::EffectSet effects;
    ukefx::EffectChain chain;
    const std::string error = build(effects, chain, {"delay", "distortion", "delay"});
    EXPECT_NE(error.find("only appear once"), std::string::npos) << error;
}
