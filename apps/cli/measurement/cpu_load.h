#pragma once

#include <thread>
#include <vector>

namespace cli {

// Keeps the machine busy, for measuring the audio thread under load (--load N).
//
// Each thread runs at normal priority, like the background programs it stands in
// for (a browser, a build), and walks a buffer larger than the CPU caches, so it
// competes for memory bandwidth and cache space as well as for CPU time. The same
// number of threads gives the same load on every run, so experiments repeat.
//
// Main thread only: start it before audio, and it stops when destroyed.
class CpuLoad {
public:
    explicit CpuLoad(int threads);
    ~CpuLoad() = default; // std::jthread asks each thread to stop, then joins it

    CpuLoad(const CpuLoad&) = delete;
    CpuLoad& operator=(const CpuLoad&) = delete;

private:
    std::vector<std::jthread> threads_;
};

} // namespace cli
