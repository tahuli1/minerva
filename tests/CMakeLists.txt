add_executable(minerva_tests
    delay_test.cpp
    denormals_test.cpp
    distortion_test.cpp
    effect_chain_test.cpp
    effect_set_test.cpp
    offline_test.cpp
    smoothed_value_test.cpp
    voice_test.cpp
    wav_io_test.cpp)
# denormals.h lives with the backends (it is x86-specific), so its folder is added here.
target_include_directories(minerva_tests PRIVATE ${PROJECT_SOURCE_DIR}/backends)
target_link_libraries(minerva_tests PRIVATE
    minerva_core minerva_offline dr_wav GTest::gtest_main minerva_warnings)

include(GoogleTest)
gtest_discover_tests(minerva_tests)
