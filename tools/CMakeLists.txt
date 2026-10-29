# Offline processing: WAV files through the same effects, with no audio device.
add_library(ukefx_offline STATIC
    offline.cpp
    offline.h
    wav_io.cpp
    wav_io.h)
target_include_directories(ukefx_offline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ukefx_offline PUBLIC ukefx_core PRIVATE dr_wav ukefx_warnings)

add_executable(ukefx_process process_wav.cpp)
target_link_libraries(ukefx_process PRIVATE ukefx_offline ukefx_warnings)
