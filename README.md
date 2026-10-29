# ukefx

A real-time audio effects engine for acoustic ukulele, written in C++20 on Windows.

Audio comes in from a microphone, passes through an effect chain, and goes out to
headphones live. The project focuses on real-time engineering: hard deadlines,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. The app runs the microphone through a chain of
effects (distortion, delay) to the headphones, with live parameter control, and the
same effects can process WAV files offline. More effects, measurements and a GUI are
coming next.

## Requirements

- Windows 10/11
- Visual Studio 2022 or newer with the C++ workload
- CMake 3.24+
- Git (CMake fetches RtAudio, GoogleTest and dr_wav on first configure)

## Build

```
cmake -S . -B build
cmake --build build --config Release
```

## Run

```
build\apps\cli\Release\ukefx_cli.exe [effect ...]
```

Effects run in the order given. Known effects: `distortion` (warm overdrive) and
`delay` (350 ms echo). With no arguments it runs `distortion delay`. Examples:

```
build\apps\cli\Release\ukefx_cli.exe delay
build\apps\cli\Release\ukefx_cli.exe delay distortion
```

While it runs, change parameters from the keyboard (lowercase lowers, uppercase raises):

| Key | Parameter |
|---|---|
| `d` / `D` | distortion drive |
| `f` / `F` | delay feedback |
| `m` / `M` | delay mix |
| `q` | quit (Ctrl+C works too) |

Changes glide over 20 ms, so they don't click.

Use headphones, and turn the volume down before the first run: with speakers, the
microphone will pick up the output and feed back.

## Process a WAV file offline

```
build\tools\Release\ukefx_process.exe <in.wav> <out.wav> [effect ...]
```

Runs a recording through the same effects as the live app, in the same 128-sample
blocks, with no audio device. The effect names work as above (default:
`distortion delay`). The output is a mono 32-bit float WAV with 2 s of extra tail so
the echoes can fade out.

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
