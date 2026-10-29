# Offline tools: WAV I/O and rendering, with no audio device.
add_library(minerva_offline STATIC
    offline.cpp
    offline.h
    wav_io.cpp
    wav_io.h)
target_include_directories(minerva_offline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_offline PUBLIC minerva_core PRIVATE dr_wav minerva_warnings)

add_executable(minerva_scale render_scale.cpp)
target_link_libraries(minerva_scale PRIVATE minerva_offline minerva_warnings)
