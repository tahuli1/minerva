# The command-line app. main.cpp wires everything together; options.* reads the
# command line.
#   keyboard/     reading keys, and turning them into notes and settings
#   display/      everything printed to the console
#   measurement/  the --load and --csv tools for experiments

# The CLI's logic with no Windows code in it (keys to notes, command-line options)
# is a library of its own, so the tests can link it without the console app.
add_library(minerva_cli_logic STATIC
    keyboard/note_keys.cpp
    keyboard/note_keys.h
    options.cpp
    options.h)
target_include_directories(minerva_cli_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_cli_logic PRIVATE minerva_warnings)

add_executable(minerva_cli
    display/display.cpp
    display/display.h
    keyboard/console_keyboard.cpp
    keyboard/console_keyboard.h
    keyboard/controls.cpp
    keyboard/controls.h
    main.cpp
    measurement/cpu_load.cpp
    measurement/cpu_load.h
    measurement/timing_csv.cpp
    measurement/timing_csv.h)
target_link_libraries(minerva_cli PRIVATE minerva_core minerva_backend_rtaudio minerva_cli_logic
                                          minerva_warnings)
