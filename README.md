# Minerva

A small real-time music studio for Windows, written in C++20: play melodies and
chords on synthesized instruments from the computer keyboard, add drums, build songs
from patterns and loops, and shape the sound with effects, live, into headphones.

The project focuses on real-time engineering: hard deadlines, sample-accurate timing,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. The engine, effect chain (distortion, delay), live
parameter control, offline rendering to WAV and the first synth voice are in place;
the full synthesizer, drums, sequencer and looper are next. Until the synth exists, the app runs the microphone
through the effects, which is how the project started (as `ukefx`, an effects
processor for a ukulele).

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
build\apps\cli\Release\minerva_cli.exe [effect ...]
```

Effects run in the order given. Known effects: `distortion` (warm overdrive) and
`delay` (350 ms echo). With no arguments it runs `distortion delay`. Examples:

```
build\apps\cli\Release\minerva_cli.exe delay
build\apps\cli\Release\minerva_cli.exe delay distortion
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

## Hear the synth voice

```
build\tools\Release\minerva_scale.exe <out.wav> <sine|triangle|saw|square> [octave] [--no-aa]
```

Renders a C major scale with one synth voice (oscillator + ADSR envelope), half a
second per note. `--no-aa` turns anti-aliasing (PolyBLEP) off; compare both in a high
octave, e.g. `6`, to hear the difference.

## Test

```
ctest --test-dir build -C Release
```

## Layout

```
core/       DSP and effects, portable C++ with no OS headers
backends/   audio device I/O (currently RtAudio over WASAPI)
apps/cli/   command-line app
tools/      offline and measurement tools
tests/      unit tests
```
