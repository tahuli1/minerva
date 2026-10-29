# Minerva — Real-Time Music Studio

## What this is
A small real-time music studio for Windows, written in C++. You play melodies and chords
on synthesized instruments from the computer keyboard, add drums, build songs from
patterns and loops, and shape the sound with effects, live, into headphones. Not tied
to one genre. The full spec and design are in `PLAN.html` (Hebrew), kept locally (not in the repo).

This is a **portfolio project for real-time (RT) developer job interviews**. The point is
not just "it makes music" — it is to demonstrate real-time engineering: hard deadlines,
determinism, sample-accurate timing, lock-free communication, and *measured*
latency/jitter improvements.

(Started as `ukefx`, an effects processor for a ukulele through a mic. Milestones 1–2
were built under that name; the engine, effects and tooling carry over.)

## Goals (definition of done)
- Round-trip latency under 10 ms.
- Zero xruns (buffer underruns/overruns) over an hour of playing, even with the system under load.
- Sample-accurate sequencing: every step lands on its exact sample, whatever the block size.
- Measurements: per-callback processing time vs. deadline (histogram + live graph), xrun count,
  measured latency, CPU cost vs. number of active voices.
- A README with before/after numbers for every RT improvement, and a demo video.

## Platform & tech
- **OS:** Windows (primary). Keep `core/` portable — a Linux backend (JACK/ALSA) and an STM32 port may come later.
- **Language:** C++20, MSVC, CMake.
- **Audio I/O:** RtAudio (over WASAPI), duplex: the mic is used only to record into the looper. Later: our own WASAPI exclusive-mode backend, then ASIO.
- **Input:** the computer keyboard (75%, no numpad), read by virtual key code with key-up events. No MIDI hardware for now.
- **GUI (later stage):** Dear ImGui + ImPlot.
- **DSP:** DaisySP (oscillators, envelopes, filters, drums, effects): MIT, written for embedded, never allocates.
- **Tests:** GoogleTest; effects and voices render into memory, so no sound card is needed (and CI can run them).
- **CI:** GitHub Actions on Windows runners.
- Dependencies via CMake FetchContent (or vcpkg if FetchContent is impractical for a dependency).

## Architecture
```
minerva/
  core/           # portable library. NO OS headers.
    dsp/          #   building blocks: smoothing, oscillators, envelopes, filters
    effects/      #   Effect interface, chain, set; one folder per effect
    synth/        #   voice, voice pool, presets
    drums/        #   drum voices
    sequencer/    #   sample-based clock, patterns
    rt/           #   lock-free SPSC queue
    engine/       #   owns everything above; one process() per audio block
  backends/       # audio device I/O (rtaudio, later wasapi, asio, linux)
  apps/
    cli/          # keyboard input + text display
    gui/          # later: ImGui front-end
  tools/          # WAV I/O, offline render, latency measurement
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
SPSC queues — commands one way (note on/off, transport, edits), status and metrics the
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

Windows RT setup for the audio thread: MMCSS ("Pro Audio"), `VirtualLock` for audio memory,
timing with `QueryPerformanceCounter`.

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
1. **Skeleton:** ✅ CMake project, RtAudio via FetchContent, CLI pass-through.
2. **Effects:** ✅ distortion, delay, effect chain, parameter smoothing, offline WAV tool, CI.
3. **Synth:** rename to Minerva, oscillators, ADSR, 16-voice pool, keyboard input (key down/up) through an SPSC command queue; runs without a mic (output-only stream when none is present).
4. **Measurement:** callback timing vs. deadline, own xrun detection, lock-free metrics buffer, CSV export + plotting script, loopback latency tool, a test that fails if the callback allocates, and an ARM (Cortex-M) build of `core/` in CI.
5. **Sounds:** filter + filter envelope, presets (keys, pad, bass, lead, pluck), reverb.
6. **Sequencer + drums:** sample-based clock, 16-step patterns, synthesized drums, recording into patterns, save/load songs (JSON, nlohmann/json), offline render to WAV (dr_wav).
7. **RT hardening:** MMCSS, `VirtualLock`, buffer-size tuning, stress testing, LatencyMon/ETW analysis. Record before/after numbers.
8. **Looper:** locked audio buffer, record/play/overdub in sync with the pattern; hold a key to record from the mic, with latency compensation.
9. **Own backend:** WASAPI exclusive mode (then ASIO if hardware arrives).
10. **GUI:** ImGui on-screen keyboard, step grid, parameters, save/open buttons, live ImPlot of callback time vs. deadline.
11. **Polish:** README with graphs, demo video, public repository.

## Build
```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```
(Update this section as the project evolves.)
