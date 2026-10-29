cmake_minimum_required(VERSION 3.24)
project(ukefx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Plain ISO C++20, not GNU dialects (-std=gnu++20 on GCC/Clang). core/ has to stay
# portable to Linux and STM32 toolchains, so compiler extensions must not sneak in.
set(CMAKE_CXX_EXTENSIONS OFF)

# Interface target that carries only compile flags, no code. Linking it applies the
# flags to that target alone, so our code gets strict warnings while third-party
# code (RtAudio) is built with its own defaults instead of flooding the build log.
add_library(ukefx_warnings INTERFACE)
if(MSVC)
    target_compile_options(ukefx_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(ukefx_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# Everything is linked statically: one self-contained exe, no DLLs to copy around.
# Set explicitly because RtAudio declares BUILD_SHARED_LIBS with a default of ON,
# which would otherwise turn GoogleTest into DLLs too.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# RtAudio: WASAPI only for now. DirectSound adds latency we never want;
# ASIO comes later together with real hardware.
set(RTAUDIO_API_WASAPI ON  CACHE BOOL "" FORCE)
set(RTAUDIO_API_DS     OFF CACHE BOOL "" FORCE)
set(RTAUDIO_API_ASIO   OFF CACHE BOOL "" FORCE)
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(RTAUDIO_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
# A static RtAudio defaults to the static CRT (/MT); keep it on /MD like our code.
set(RTAUDIO_STATIC_MSVCRT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG        6.0.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(rtaudio)

# GoogleTest on the dynamic CRT (/MD) like the rest of the build.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(googletest)

# dr_wav: a single public-domain header for reading and writing WAV files offline.
# Only the header is used; SOURCE_SUBDIR points at nothing so dr_libs' own CMake
# (its tests and other libraries) is never configured.
FetchContent_Declare(dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG        wav-0.14.5
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  no-cmake)
FetchContent_MakeAvailable(dr_libs)
add_library(dr_wav INTERFACE)
# SYSTEM: warnings from inside the header are not ours to fix, so /W4 skips them.
target_include_directories(dr_wav SYSTEM INTERFACE ${dr_libs_SOURCE_DIR})

enable_testing()

add_subdirectory(core)
add_subdirectory(backends)
add_subdirectory(apps/cli)
add_subdirectory(tools)
add_subdirectory(tests)
