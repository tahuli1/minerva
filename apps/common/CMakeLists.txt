# Logic both apps share, with no Windows code in it, so the tests link it too:
#   keyboard/   keys to notes (NoteKeys) and keys to settings (controls)
add_library(minerva_app_common STATIC
    keyboard/controls.cpp
    keyboard/controls.h
    keyboard/note_keys.cpp
    keyboard/note_keys.h)
target_include_directories(minerva_app_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# PUBLIC minerva_core: controls.h hands settings to the Engine.
target_link_libraries(minerva_app_common PUBLIC minerva_core PRIVATE minerva_warnings)
