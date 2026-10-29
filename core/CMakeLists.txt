# Portable DSP library: standard C++ only, no OS or audio-API headers.
#   dsp/      small building blocks of our own (parameter smoothing)
#   effects/  the effect framework (interface, chain) at its root,
#             and one folder per concrete effect
#   synth/    the synthesizer: voices built from DaisySP, and the voice pool
#   engine/   owns the synth, the effect chain and the note-command queue
add_library(minerva_core STATIC
    dsp/smoothed_value.h
    effects/delay/delay.cpp
    effects/delay/delay.h
    effects/distortion/distortion.cpp
    effects/distortion/distortion.h
    effects/effect.h
    effects/effect_chain.cpp
    effects/effect_chain.h
    engine/engine.cpp
    engine/engine.h
    synth/synth.cpp
    synth/synth.h
    synth/voice.cpp
    synth/voice.h)
target_include_directories(minerva_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# PUBLIC: core's headers hold DaisySP objects and the SPSC queue, so users need them too.
target_link_libraries(minerva_core PUBLIC daisysp spscqueue PRIVATE minerva_warnings)
