add_library(minerva_backend_rtaudio STATIC
    denormals.h
    rtaudio/rtaudio_backend.cpp
    rtaudio/rtaudio_backend.h)
target_include_directories(minerva_backend_rtaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# PRIVATE: RtAudio types stay out of the public header, so apps don't depend on RtAudio.
target_link_libraries(minerva_backend_rtaudio PRIVATE rtaudio minerva_warnings)
