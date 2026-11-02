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

Every audio callback is timed. To see the measurements, start the app with
`--stats`:

```
build\apps\cli\Release\minerva_cli.exe --stats
```

Under CALLBACK TIMING it then adds one row per second:

```
  time |  avg us | p99.9 us |  max us | load % | gap max ms | xruns | dropped
-------+---------+----------+---------+--------+------------+-------+--------
    1s |     2.1 |     10.4 |    10.4 |    0.4 |       2.70 |     0 |       0
    2s |     5.3 |     11.0 |    25.1 |    0.9 |       2.75 |     0 |       0
```

- `avg`, `p99.9`, `max`: how long the callbacks took, in microseconds.
- `load %`: the slowest callback as a share of the deadline, which is one block
  (128 frames at 48 kHz = 2.67 ms). What is left is the headroom.
- `gap max`: the longest time between the starts of two callbacks. Much more than
  one block means something outside the app (the OS, the driver) held the audio
  thread back.
- `xruns`: blocks the driver flagged as late in that second.
- `dropped`: timings lost because the display fell behind (the audio thread never
  waits for it).

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
