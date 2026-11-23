# Minerva — Real-Time Synth

## What this is
A small real-time synthesizer for Windows, written in C++. You play melodies and chords
from the computer keyboard, through effects, live into headphones. The full spec and
design are in `PLAN.html` (Hebrew), kept locally (not in the repo).

This is a **portfolio project for real-time (RT) developer job interviews**, kept small
on purpose, and developed with AI assistance. The point is not just "it makes music" —
it is to demonstrate real-time engineering: hard deadlines, determinism, lock-free
communication, and *measured* timing and jitter, including under load.

(Started as `ukefx`, an effects processor for a ukulele through a mic. Milestones 1–2
were built under that name; the engine, effects and tooling carry over.)

## Goals (definition of done)
- Zero late callbacks in the load experiment (30 s per block size, with one busy thread per CPU core).
- Measurements: per-callback processing time vs. deadline (histogram + live graph), late-callback count.
- A README with before/after numbers for every RT improvement, and a demo video.
- Bonus: round-trip latency under 10 ms, measured with a loopback tool.

## Platform & tech
- **OS:** Windows (primary). Keep `core/` free of OS headers, so it is tested without a sound card and a Linux backend (JACK/ALSA) could come later.
- **Language:** C++20, MSVC, CMake.
- **Audio I/O:** RtAudio (over WASAPI), output-only. The mic would only matter for the looper (a bonus milestone), so it is not opened. Bonus: our own WASAPI exclusive-mode backend, then ASIO.
- **Input:** the computer keyboard (75%, no numpad), read by virtual key code with key-up events. No MIDI hardware for now.
- **GUI:** Dear ImGui + ImPlot over Direct3D 11 (Win32 backend).
- **DSP:** DaisySP (oscillators, envelopes, filters, drums, effects): MIT, written for embedded, never allocates.
- **Tests:** GoogleTest; effects and voices render into memory, so no sound card is needed (and CI can run them).
- **CI:** GitHub Actions on Windows runners.
- Dependencies via CMake FetchContent (or vcpkg if FetchContent is impractical for a dependency).

## Architecture
```
minerva/
  core/           # portable library. NO OS headers.
    dsp/          #   building blocks: parameter smoothing
    effects/      #   Effect interface and chain; one folder per effect
    synth/        #   voice, voice pool (oscillator + envelope from DaisySP)
    rt/           #   callback timing and its statistics
    engine/       #   owns everything above; one process() per audio block
  backends/       # audio device I/O (RtAudio over WASAPI)
  apps/
    common/       # key layout and controls shared by both apps (keyboard/)
    cli/          # console input + text display (keyboard/, display/, measurement/)
    gui/          # ImGui window (window/, theme/, widgets/, panels/, plot/, assets/)
  cmake/          # patches applied to fetched dependencies
  tools/          # the buffer-size experiment script
  tests/
```

Effect interface:
```cpp
class Effect {
public:
    virtual void prepare(int sampleRate, int maxBlockSize) = 0; // allocation allowed here
    virtual void process(const float* in, float* out, int n) = 0; // RT: see rules below
    virtual ~Effect() = default;
};
```

Threads: the audio thread (RT) and the control thread (keyboard, display) talk through
SPSC queues — commands one way (note on/off, waveform), callback timings the
other — plus atomics for continuous parameters, smoothed on the audio thread.

## Real-time rules (non-negotiable)
Inside the audio callback and anything it calls (`process()` etc.):
- No heap allocation/free (`new`, `malloc`, `std::vector` growth, `std::string`, `std::function` that allocates).
- No locks (`std::mutex`, critical sections), no waiting, no sleeping.
- No I/O: no `printf`/`std::cout`, no file or network access, no logging directly.
- No exceptions thrown.
- Communication with other threads only via `std::atomic` or a lock-free SPSC queue.
- Logging/metrics: write into a lock-free ring buffer, drained by a non-RT thread.
- Timing comes from the sample counter, never from OS timers or `sleep`.
- Voices, patterns, buffers and queues are sized before audio starts; per-block work has a fixed upper bound.

Windows RT setup for the audio thread: MMCSS ("Pro Audio"), timing with `QueryPerformanceCounter`.

If a change might violate these rules, stop and point it out instead of doing it.

## Coding style
- Keep things simple. Prefer the straightforward solution over a clever or abstract one.
- No premature abstraction — add layers only when a second real use case exists.
- Match the existing code style and naming in the repo.
- Small files, small functions, clear names. Comments explain *why*, not *what*.

## How to work with me
- Work in small, reviewable steps. One milestone at a time; don't jump ahead.
- After each step: explain briefly what you did and why, and how to build/run/test it.
- Explain RT-relevant decisions — I want to understand them well enough to discuss in an interview.
- Don't reinvent the wheel: use existing capabilities and third-party libraries;
  write our own code only for what no suitable library provides.
- Ask before adding a new dependency or changing the architecture.
- Git: a branch and a PR per coherent chunk of a milestone; separate logical commits.
- **No commit without my explicit approval.** Show me what changed and the planned
  commit message(s), then wait for my OK. Push and open PRs only when I say so.
- Talk to me in Hebrew; code, comments and docs in English.

## Milestones
The project stays small: what is built (1–4), then 5 → 6 → 7, and it is done. Bonus
milestones (A–D) are optional: only if there is time, never a requirement for "done".

1. **Skeleton:** ✅ CMake project, RtAudio via FetchContent, CLI pass-through.
2. **Effects:** ✅ distortion, delay, effect chain, parameter smoothing, offline WAV tool, CI.
3. **Synth:** ✅ rename to Minerva, oscillators, ADSR, 16-voice pool, keyboard input (key down/up) through an SPSC command queue; runs without a mic (output-only stream when none is present).
4. **Measurement:** ✅ callback timing vs. deadline through a lock-free queue, shown with `--stats`; late-callback detection from callback gaps; a test that fails if the callback allocates. *Bonus:* loopback latency tool, CPU cost vs. number of voices.
5. **RT hardening:** ✅ does the audio thread hold its deadline under load, and what do bigger blocks buy? Measure callback timing and late callbacks for 128, 256 and 512-frame blocks, with no load and with heavy CPU load. 128 frames (2.67 ms) is the floor in WASAPI shared mode: Windows mixes on that period, and smaller requests only split each period into several back-to-back callbacks, without lowering latency (going lower needs exclusive mode, bonus D). RtAudio already puts its WASAPI thread under MMCSS ("Pro Audio"); the app checks and shows it. A CSV export of every callback and a plotting script show the results.
6. **GUI:** ✅ Dear ImGui window: on-screen keyboard, waveform and effect parameters, and a live ImPlot graph of callback time vs. deadline with the late-callback count.
7. **Polish:** README with the architecture, the before/after graphs, the bug found by stress testing and a "How this was built" note on AI-assisted development; a short demo video; public repository.

Bonus milestones:
- **A. Sounds:** filter + filter envelope, presets (keys, pad, bass, lead, pluck), reverb.
- **B. Sequencer + drums:** sample-based clock, a 16-step drum pattern, every step on its exact sample whatever the block size.
- **C. Looper:** locked audio buffer, record/play/overdub in sync with the pattern; hold a key to record from the mic, with latency compensation.
- **D. Own backend:** WASAPI exclusive mode (then ASIO if hardware arrives).

## Build
```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```
(Update this section as the project evolves.)
