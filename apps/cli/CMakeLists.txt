# The key-to-note logic is a library of its own so the tests can link it without
# the console app around it.
add_library(minerva_note_keys STATIC
    note_keys.cpp
    note_keys.h)
target_include_directories(minerva_note_keys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(minerva_note_keys PRIVATE minerva_warnings)

add_executable(minerva_cli
    console_keyboard.cpp
    console_keyboard.h
    controls.cpp
    controls.h
    display.cpp
    display.h
    main.cpp)
target_link_libraries(minerva_cli PRIVATE minerva_core minerva_backend_rtaudio minerva_note_keys
                                          minerva_warnings)
