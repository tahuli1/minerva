add_executable(ukefx_tests
    delay_test.cpp
    denormals_test.cpp
    distortion_test.cpp
    effect_chain_test.cpp)
# denormals.h lives with the backends (it is x86-specific), so its folder is added here.
target_include_directories(ukefx_tests PRIVATE ${PROJECT_SOURCE_DIR}/backends)
target_link_libraries(ukefx_tests PRIVATE ukefx_core GTest::gtest_main ukefx_warnings)

include(GoogleTest)
gtest_discover_tests(ukefx_tests)
