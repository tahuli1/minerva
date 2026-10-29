# Minerva

A small real-time music studio for Windows, written in C++20: play melodies and
chords on synthesized instruments from the computer keyboard, add drums, build songs
from patterns and loops, and shape the sound with effects, live, into headphones.

The project focuses on real-time engineering: hard deadlines, sample-accurate timing,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. The engine, the master effect chain (distortion →
delay), live parameter control and the first synth voice (built on DaisySP) are in
place; playing the synth from the keyboard, drums, sequencer and looper are next.
Until the synth is connected, the only sound source is the microphone, if there is
one, which is how the project started (as `ukefx`, an effects processor for a ukulele).

## Requirements

- Windows 10/11
- Visual Studio 2022 or newer with the C++ workload
- CMake 3.24+
- Git (CMake fetches RtAudio, GoogleTest and DaisySP on first configure)

## Build

```
cmake -S . -B build
cmake --build build --config Release
```

## Run

```
build\apps\cli\Release\minerva_cli.exe
```

The sound goes through Minerva's fixed effect chain: distortion (warm overdrive),
then delay (350 ms echo). A microphone is optional: without one the app still runs,
output only.

While it runs, change the effects from the function keys (the letter keys are kept
free for playing notes):

| Key | Parameter |
|---|---|
| `F1` / `F2` | distortion drive down / up |
| `F3` / `F4` | delay feedback down / up |
| `F5` / `F6` | delay mix down / up |
| `Esc` | quit (Ctrl+C works too) |

On some keyboards the F keys need the `Fn` key. Changes glide over 20 ms, so they
don't click.

Use headphones, and turn the volume down before the first run: with speakers, the
microphone will pick up the output and feed back.

## Test

```
ctest --test-dir build -C Release
```

## Layout

```
core/       DSP and effects, portable C++ with no OS headers
backends/   audio device I/O (currently RtAudio over WASAPI)
apps/cli/   command-line app
tests/      unit tests
```
