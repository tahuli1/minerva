# Portable DSP library: standard C++ only, no OS or audio-API headers.
#   dsp/      small building blocks of our own (parameter smoothing)
#   effects/  the effect framework (interface, chain, set) at its root,
#             and one folder per concrete effect
#   synth/    the synthesizer: voices built from DaisySP oscillators and envelopes
add_library(minerva_core STATIC
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
# PUBLIC: voice.h holds DaisySP objects, so users of core need its headers too.
target_link_libraries(minerva_core PUBLIC daisysp PRIVATE minerva_warnings)
