# Minerva

A small real-time music studio for Windows, written in C++20: play melodies and
chords on synthesized instruments from the computer keyboard, add drums, build songs
from patterns and loops, and shape the sound with effects, live, into headphones.

The project focuses on real-time engineering: hard deadlines, sample-accurate timing,
no allocation or locking on the audio thread, and measured latency.

**Status:** early development. A 16-voice synth (built on DaisySP) plays from the
computer keyboard through the master effect chain (distortion → delay); presets,
drums, sequencer and looper are next.

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

Play the synth from the computer keyboard, laid out like a piano:

```
 W E   T Y U   O P       black keys
A S D F G H J K L ;      white keys: C D E F G A B C D E
```

`A` is middle C. Hold several keys for a chord. The keys are read by position, so
they work whatever the keyboard language is.

Run it in a console window (cmd, PowerShell or Windows Terminal). Terminals
embedded in editors, such as VS Code's, pass on characters rather than keys, so
key releases never arrive and notes don't play properly.

| Key | Action |
|---|---|
| `Z` / `X` | octave down / up |
| `C` / `V` | velocity (loudness of the next notes) down / up |
| `1` – `4` | waveform: sine, triangle, saw, square |
| `F7` | distortion on / off (off at start: it makes chords sound harsh) |
| `F1` / `F2` | distortion drive down / up |
| `F3` / `F4` | delay feedback down / up |
| `F5` / `F6` | delay mix down / up |
| `Esc` | quit (Ctrl+C works too) |

On some keyboards the F keys need the `Fn` key. Effect changes glide over 20 ms, so
they don't click.

A microphone is not needed; it will be used for recording into the looper.

Turn the volume down before the first run.

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
