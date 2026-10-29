# Portable DSP library: standard C++ only, no OS or audio-API headers.
#   dsp/      building blocks shared by everything (smoothing, oscillator, envelope)
#   effects/  the effect framework (interface, chain, set) at its root,
#             and one folder per concrete effect
#   synth/    the synthesizer, built from dsp/ blocks (voice; later the voice pool)
add_library(minerva_core STATIC
    dsp/envelope.cpp
    dsp/envelope.h
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
    effects/effect_set.h
    synth/voice.cpp
    synth/voice.h)
target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_core PRIVATE minerva_warnings)
