# Portable DSP library: standard C++ only, no OS or audio-API headers.
add_library(ukefx_core STATIC
    effect.h
    delay.cpp
    delay.h
    distortion.cpp
    distortion.h)
target_include_directories(ukefx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ukefx_core PRIVATE ukefx_warnings)
