add_executable(ukefx_tests
    distortion_test.cpp)
target_link_libraries(ukefx_tests PRIVATE ukefx_core GTest::gtest_main ukefx_warnings)

include(GoogleTest)
gtest_discover_tests(ukefx_tests)
