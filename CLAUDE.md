# ukefx — Real-Time Ukulele Effects Engine

## What this is
A real-time audio effects processor for an acoustic ukulele, written in C++ on Windows.
Audio comes in (laptop mic now, later a piezo pickup + USB audio interface), passes through
an effect chain (distortion, delay, reverb, looper), and goes out to headphones live.

This is a **portfolio project for real-time (RT) developer job interviews**. The point is not
just "it makes sound" — it is to demonstrate real-time engineering: hard deadlines,
determinism, lock-free communication, and *measured* latency/jitter improvements.

## Goals (definition of done)
- Round-trip latency under 10 ms.
- Zero xruns (buffer underruns/overruns) over an hour of playing, even with the system under load.
- Measurements: per-callback processing time vs. deadline (histogram + live graph), xrun count, measured latency.
- A README with before/after numbers for every RT improvement, and a demo video.

## Platform & tech
- **OS:** Windows (primary). Keep `core/` portable — a Linux backend (JACK/ALSA) and an STM32 port may come later.
- **Language:** C++20, MSVC, CMake.
- **Audio I/O:** start with RtAudio (over WASAPI). Later: our own WASAPI exclusive-mode backend, then ASIO.
- **GUI (later stage):** Dear ImGui + ImPlot.
- **Tests:** GoogleTest; offline effect tests run on WAV files via libsndfile.
- **CI:** GitHub Actions on Windows runners.
- Dependencies via CMake FetchContent (or vcpkg if FetchContent is impractical for a dependency).

## Architecture
```
ukefx/
  core/       # library: Effect interface, effect chain, effects, lock-free queue, params. NO OS headers.
  backends/   # audio device I/O (rtaudio, later wasapi, asio, linux)
  apps/
    cli/      # first app: run from terminal
    gui/      # later: ImGui front-end
  tools/      # offline WAV processor, latency measurement
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

## Real-time rules (non-negotiable)
Inside the audio callback and anything it calls (`process()` etc.):
- No heap allocation/free (`new`, `malloc`, `std::vector` growth, `std::string`, `std::function` that allocates).
- No locks (`std::mutex`, critical sections), no waiting, no sleeping.
- No I/O: no `printf`/`std::cout`, no file or network access, no logging directly.
- No exceptions thrown.
- Communication with other threads only via `std::atomic` or a lock-free SPSC queue.
- Logging/metrics: write into a lock-free ring buffer, drained by a non-RT thread.

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
- Ask before adding a new dependency or changing the architecture.
- Talk to me in Hebrew; code, comments and docs in English.

## Milestones
1. **Skeleton:** CMake project, RtAudio via FetchContent, CLI app with mic → headphones pass-through.
2. **First effects:** distortion (`tanh`), then delay (circular buffer). Effect chain. Offline WAV tests.
3. **Measurement:** callback timing vs. deadline, xrun counter, lock-free metrics buffer, CSV export + plotting script.
4. **RT hardening:** MMCSS, `VirtualLock`, buffer-size tuning, stress testing, LatencyMon/ETW analysis. Record before/after numbers.
5. **Own backend:** WASAPI exclusive mode (then ASIO when hardware arrives).
6. **GUI:** ImGui effect chain editor, parameters, presets (JSON), live ImPlot of callback time vs. deadline.
7. **Polish:** README with graphs, demo video, CI.

## Build
```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
build\apps\cli\Release\ukefx_cli.exe   # mic -> headphones pass-through, Ctrl+C to stop
```
Generator: Visual Studio 18 2026 (CMake default on this machine). RtAudio is built static, WASAPI only.
(Update this section as the project evolves.)
