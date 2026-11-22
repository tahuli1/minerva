# Minerva

A small real-time synthesizer for Windows, written in C++20: play melodies and
chords from the computer keyboard, through distortion and delay, live into
headphones.

The project is about real-time engineering: a hard deadline every 2.67 ms, no
allocation or locking on the audio thread, lock-free communication between
threads, and measured timing, including under heavy CPU load.

**Status:** a 16-voice synth (built on DaisySP) plays from the keyboard, every
audio callback is measured, and a test proves the audio path never allocates. A
GUI comes next.

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
 W E   T Y U   O P   ]     black keys
A S D F G H J K L ; '      white keys: C D E F G A B C D E F
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

Minerva only plays: it opens the speakers or headphones, never the microphone.

Every audio callback is timed. To see the measurements, start the app with
`--stats`:

```
build\apps\cli\Release\minerva_cli.exe --stats
```

Under CALLBACK TIMING it then adds one row per second:

```
  time |  avg us | p99.9 us |  max us | load % | gap max ms | late | dropped
-------+---------+----------+---------+--------+------------+------+--------
    1s |     2.1 |     10.4 |    10.4 |    0.4 |       2.70 |    0 |       0
    2s |     5.3 |     11.0 |    25.1 |    0.9 |       2.75 |    0 |       0
```

- `avg`, `p99.9`, `max`: how long the callbacks took, in microseconds.
- `load %`: the slowest callback as a share of the deadline, which is one block
  (128 frames at 48 kHz = 2.67 ms). What is left is the headroom.
- `gap max`: the longest time between the starts of two callbacks. Much more than
  one block means something outside the app (the OS, the driver) held the audio
  thread back.
- `late`: callbacks that started more than 1.5 block periods after the previous
  one. A late callback may be a click, but not necessarily: Windows keeps some
  audio queued and may cover the delay. Windows (WASAPI) does not tell an app when
  its output ran dry, so this is the only warning there is.
- `dropped`: timings lost because the display fell behind (the audio thread never
  waits for it).

Turn the volume down before the first run.

Options for measuring (all optional):

| Option | What it does |
|---|---|
| `--stats` | show the callback timing table |
| `--buffer FRAMES` | block size to ask the device for (default 128) |
| `--load THREADS` | keep the CPU busy with that many threads at normal priority |
| `--csv FILE` | write every callback's timing to a CSV file |
| `--seconds N` | quit after N seconds, for unattended runs |

At startup the AUDIO section shows the audio thread's priority. RtAudio's WASAPI
thread joins MMCSS as "Pro Audio", and it shows as priority 15 (normal threads
are 0).

## Experiment: does it hold the deadline under load?

`tools/buffer_experiment.py` runs the app for 128, 256 and 512-frame blocks, with
no load and with one busy thread per CPU core, and charts late callbacks, the
longest gap between callbacks, and the jitter at 128 frames. It needs Python with
matplotlib (`python -m pip install matplotlib`):

```
python tools/buffer_experiment.py --seconds 30
```

Results go to `results/`. Why it stops at 128 frames: in WASAPI shared mode
Windows mixes every 2.67 ms (128 frames at 48 kHz). Asking for a smaller block
only makes RtAudio split each period into several back-to-back callbacks; the
sound still leaves at the same pace, so latency does not drop. Going lower needs
WASAPI exclusive mode.

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
tools/      the buffer-size experiment script
```
