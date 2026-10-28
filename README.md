# ukefx

A real-time audio effects engine for acoustic ukulele, written in C++20 on Windows.

Audio comes in from a microphone, passes through an effect chain, and goes out to
headphones live. The project focuses on real-time engineering: hard deadlines,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. Right now the app runs the microphone through a
distortion effect to the headphones. More effects, measurements and a GUI are coming next.

## Requirements

- Windows 10/11
- Visual Studio 2022 or newer with the C++ workload
- CMake 3.24+
- Git (CMake fetches RtAudio and GoogleTest on first configure)

## Build

```
cmake -S . -B build
cmake --build build --config Release
```

## Run

```
build\apps\cli\Release\ukefx_cli.exe [drive]
build\apps\cli\Release\ukefx_cli.exe delay
```

The first form runs the distortion; `drive` sets how hard it is pushed (1 is almost
clean, the default is 10). The second runs a 350 ms echo instead. For now only one
effect runs at a time.

Use headphones, and turn the volume down before the first run: with speakers, the
microphone will pick up the output and feed back. The app prints the devices and
stream settings it got, then runs until you press Ctrl+C.

## Test

```
ctest --test-dir build -C Release
```

## Layout

```
core/       effects and DSP, portable C++ with no OS headers
backends/   audio device I/O (currently RtAudio over WASAPI)
apps/cli/   command-line app
tools/      offline and measurement tools
tests/      unit tests
```
