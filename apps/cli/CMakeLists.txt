add_executable(ukefx_cli
    controls.cpp
    controls.h
    main.cpp)
target_link_libraries(ukefx_cli PRIVATE ukefx_core ukefx_backend_rtaudio ukefx_warnings)
