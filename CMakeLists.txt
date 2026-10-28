cmake_minimum_required(VERSION 3.24)
project(ukefx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings for our own targets only; third-party code is built with its defaults.
add_library(ukefx_warnings INTERFACE)
if(MSVC)
    target_compile_options(ukefx_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(ukefx_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

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

add_subdirectory(backends)
add_subdirectory(apps/cli)
