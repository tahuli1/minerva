#pragma once

#include "effects/delay/delay.h"
#include "effects/distortion/distortion.h"
#include "effects/effect_chain.h"

#include <span>
#include <string>
#include <string_view>

namespace ukefx {

// One instance of every effect, findable by name, so an app can build a chain from
// user input (the command line now, presets later). Not RT: used while setting up.
struct EffectSet {
    Distortion distortion;
    Delay delay;

    // Settings that sound reasonable out of the box, shared by every app.
    EffectSet() {
        distortion.setDrive(4.0f); // warm overdrive rather than fuzz
        distortion.setLevel(0.3f); // tanh saturates near full scale; keep headphones safe
    }

    // Returns nullptr for an unknown name.
    Effect* find(std::string_view name);

    static constexpr std::string_view kKnownNames = "distortion, delay";
};

// Adds the named effects to the chain, in order. Each effect may appear once: one
// object in two slots would share its state (one delay buffer written twice per
// sample). Returns an error message, or an empty string on success.
std::string buildChain(EffectSet& effects, std::span<const std::string_view> names,
                       EffectChain& chain);

} // namespace ukefx
