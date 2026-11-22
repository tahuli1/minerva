# The CLI's logic with no Windows code in it (keys to notes, command-line options)
# is a library of its own, so the tests can link it without the console app.
add_library(minerva_cli_logic STATIC
    note_keys.cpp
    note_keys.h
    options.cpp
    options.h)
target_include_directories(minerva_cli_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_cli_logic PRIVATE minerva_warnings)

add_executable(minerva_cli
    console_keyboard.cpp
    console_keyboard.h
    controls.cpp
    controls.h
    cpu_load.cpp
    cpu_load.h
    display.cpp
    display.h
    main.cpp
    timing_csv.cpp
    timing_csv.h)
target_link_libraries(minerva_cli PRIVATE minerva_core minerva_backend_rtaudio minerva_cli_logic
                                          minerva_warnings)
