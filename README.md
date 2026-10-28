# ukefx

A real-time audio effects engine for acoustic ukulele, written in C++20 on Windows.

Audio comes in from a microphone, passes through an effect chain, and goes out to
headphones live. The project focuses on real-time engineering: hard deadlines,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. Right now the app passes the microphone straight
through to the headphones. Effects, measurements and a GUI are coming next.

## Requirements

- Windows 10/11
- Visual Studio 2022 or newer with the C++ workload
- CMake 3.24+
- Git (CMake fetches RtAudio on first configure)

## Build

```
cmake -S . -B build
cmake --build build --config Release
```

## Run

```
build\apps\cli\Release\ukefx_cli.exe
```

Use headphones: with speakers, the microphone will pick up the output and feed back.
The app prints the devices and stream settings it got, then runs until you press Ctrl+C.

## Layout

```
core/       effects and DSP, portable C++ with no OS headers
backends/   audio device I/O (currently RtAudio over WASAPI)
apps/cli/   command-line app
tools/      offline and measurement tools
tests/      unit tests
```
