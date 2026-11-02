add_executable(minerva_tests
    delay_test.cpp
    denormals_test.cpp
    distortion_test.cpp
    effect_chain_test.cpp
    engine_test.cpp
    note_keys_test.cpp
    smoothed_value_test.cpp
    synth_test.cpp
    timing_stats_test.cpp
    voice_test.cpp)
# denormals.h lives with the backends (it is x86-specific), so its folder is added here.
target_include_directories(minerva_tests PRIVATE ${PROJECT_SOURCE_DIR}/backends)
target_link_libraries(minerva_tests PRIVATE minerva_core minerva_note_keys GTest::gtest_main
                                            minerva_warnings)

include(GoogleTest)
gtest_discover_tests(minerva_tests)
