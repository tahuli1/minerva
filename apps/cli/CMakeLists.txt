# The command-line app. main.cpp wires everything together; options.* reads the
# command line. The key logic it shares with the GUI is in apps/common.
#   keyboard/     reading keys from the console
#   display/      everything printed to the console
#   measurement/  the --load and --csv tools for experiments

# Command-line parsing has no Windows code in it, so it is a library of its own
# that the tests can link without the console app.
add_library(minerva_cli_logic STATIC
    options.cpp
    options.h)
target_include_directories(minerva_cli_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_cli_logic PRIVATE minerva_warnings)

add_executable(minerva_cli
    display/display.cpp
    display/display.h
    keyboard/console_keyboard.cpp
    keyboard/console_keyboard.h
    main.cpp
    measurement/cpu_load.cpp
    measurement/cpu_load.h
    measurement/timing_csv.cpp
    measurement/timing_csv.h)
target_link_libraries(minerva_cli PRIVATE minerva_core minerva_backend_rtaudio minerva_app_common
                                          minerva_cli_logic minerva_warnings)
