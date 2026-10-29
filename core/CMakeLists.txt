# Portable DSP library: standard C++ only, no OS or audio-API headers.
#   dsp/      building blocks shared by everything (smoothing, oscillator)
#   effects/  the effect framework (interface, chain, set) at its root,
#             and one folder per concrete effect
add_library(minerva_core STATIC
    dsp/oscillator.cpp
    dsp/oscillator.h
    dsp/smoothed_value.h
    effects/delay/delay.cpp
    effects/delay/delay.h
    effects/distortion/distortion.cpp
    effects/distortion/distortion.h
    effects/effect.h
    effects/effect_chain.cpp
    effects/effect_chain.h
    effects/effect_set.cpp
    effects/effect_set.h)
target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_core PRIVATE minerva_warnings)
