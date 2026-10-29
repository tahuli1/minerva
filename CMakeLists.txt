cmake_minimum_required(VERSION 3.24)
project(minerva LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Plain ISO C++20, not GNU dialects (-std=gnu++20 on GCC/Clang). core/ has to stay
# portable to Linux and STM32 toolchains, so compiler extensions must not sneak in.
set(CMAKE_CXX_EXTENSIONS OFF)

# Interface target that carries only compile flags, no code. Linking it applies the
# flags to that target alone, so our code gets strict warnings while third-party
# code (RtAudio) is built with its own defaults instead of flooding the build log.
add_library(minerva_warnings INTERFACE)
if(MSVC)
    target_compile_options(minerva_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(minerva_warnings INTERFACE -Wall -Wextra -Wpedantic)
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

# DaisySP: DSP building blocks (oscillators, envelopes, filters, drums) written for
# embedded audio, so they never allocate. MIT licensed.
# We build only the files we use instead of DaisySP's own CMake target: some of its
# other files use GCC-only syntax that MSVC rejects. SOURCE_SUBDIR points at nothing
# so that target is never configured.
FetchContent_Declare(daisysp
    GIT_REPOSITORY https://github.com/electro-smith/DaisySP.git
    GIT_TAG        V1.0.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  no-cmake)
FetchContent_MakeAvailable(daisysp)
set(DAISYSP_SRC ${daisysp_SOURCE_DIR}/Source)
add_library(daisysp STATIC
    ${DAISYSP_SRC}/Control/adsr.cpp
    ${DAISYSP_SRC}/Synthesis/oscillator.cpp)
target_include_directories(daisysp SYSTEM PUBLIC ${DAISYSP_SRC})
# Its sources include each other by bare file name, from these folders.
target_include_directories(daisysp PRIVATE ${DAISYSP_SRC}/Control ${DAISYSP_SRC}/Synthesis
                                           ${DAISYSP_SRC}/Utility)
# MSVC only defines M_E, M_PI etc. when asked to.
target_compile_definitions(daisysp PRIVATE _USE_MATH_DEFINES)
if(MSVC)
    target_compile_options(daisysp PRIVATE /wd4244) # its own int -> float conversions
endif()

enable_testing()

add_subdirectory(core)
add_subdirectory(backends)
add_subdirectory(apps/cli)
add_subdirectory(tests)
