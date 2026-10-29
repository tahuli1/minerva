# Offline processing: WAV files through the same effects, with no audio device.
add_library(minerva_offline STATIC
    offline.cpp
    offline.h
    wav_io.cpp
    wav_io.h)
target_include_directories(minerva_offline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_offline PUBLIC minerva_core PRIVATE dr_wav minerva_warnings)

add_executable(minerva_process process_wav.cpp)
target_link_libraries(minerva_process PRIVATE minerva_offline minerva_warnings)
