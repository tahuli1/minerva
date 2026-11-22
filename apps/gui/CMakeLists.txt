# The GUI app: a Dear ImGui window over Direct3D 11. main.cpp wires the engine,
# the audio backend and the frame loop together.
#   window/   the Win32 window, the Direct3D 11 device, and loading the logo
#   theme/    colors, spacing and fonts from the mockup
#   widgets/  controls ImGui lacks: knobs, waveform pads, switches, stat cards
#   panels/   the window's sections: header, SOUND, EFFECTS, ENGINE, KEYS, the graph
#   plot/     the graph's history of callback timings
#   assets/   the logo, built into the exe by minerva_gui.rc
add_executable(minerva_gui WIN32
    main.cpp
    minerva_gui.rc
    panels/panels.cpp
    panels/panels.h
    plot/timing_history.cpp
    plot/timing_history.h
    resource.h
    theme/theme.cpp
    theme/theme.h
    widgets/widgets.cpp
    widgets/widgets.h
    window/graphics_window.cpp
    window/graphics_window.h
    window/texture.cpp
    window/texture.h)
target_include_directories(minerva_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# windowscodecs and ole32: WIC and COM, to read the logo.
target_link_libraries(minerva_gui PRIVATE minerva_core minerva_backend_rtaudio minerva_app_common
                                          imgui implot windowscodecs ole32 minerva_warnings)
