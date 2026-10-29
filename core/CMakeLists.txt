# Portable DSP library: standard C++ only, no OS or audio-API headers.
# Top level: the framework (Effect, EffectChain). effects/: one effect per file pair.
# dsp/: building blocks shared by effects.
add_library(ukefx_core STATIC
    dsp/smoothed_value.h
    effect.h
    effect_chain.cpp
    effect_chain.h
    effect_set.cpp
    effect_set.h
    effects/delay.cpp
    effects/delay.h
    effects/distortion.cpp
    effects/distortion.h)
target_include_directories(ukefx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ukefx_core PRIVATE ukefx_warnings)
