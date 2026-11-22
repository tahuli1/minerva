# The GUI app: a Dear ImGui window over Direct3D 11. main.cpp wires the engine,
# the audio backend and the frame loop together.
#   window/   the Win32 window and the Direct3D 11 device ImGui draws with
add_executable(minerva_gui WIN32
    main.cpp
    window/graphics_window.cpp
    window/graphics_window.h)
target_include_directories(minerva_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_gui PRIVATE minerva_core minerva_backend_rtaudio minerva_app_common
                                          imgui implot minerva_warnings)
