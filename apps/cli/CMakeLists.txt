add_executable(minerva_cli
    controls.cpp
    controls.h
    main.cpp)
target_link_libraries(minerva_cli PRIVATE minerva_core minerva_backend_rtaudio minerva_warnings)
