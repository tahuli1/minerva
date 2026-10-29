#include "effects/effect_set.h"

#include <algorithm>
#include <array>

namespace ukefx {

Effect* EffectSet::find(std::string_view name) {
    if (name == "distortion") return &distortion;
    if (name == "delay") return &delay;
    return nullptr;
}

std::string buildChain(EffectSet& effects, std::span<const std::string_view> names,
                       EffectChain& chain) {
    std::array<const Effect*, EffectChain::kMaxEffects> added{};
    int count = 0;
    for (std::string_view name : names) {
        Effect* effect = effects.find(name);
        if (effect == nullptr)
            return "unknown effect '" + std::string(name) + "'. Known effects: " +
                   std::string(EffectSet::kKnownNames);
        if (std::find(added.begin(), added.begin() + count, effect) != added.begin() + count)
            return "effect '" + std::string(name) + "' can only appear once";
        if (!chain.add(*effect)) return "too many effects in the chain";
        added[count++] = effect;
    }
    return {};
}

} // namespace ukefx
