#pragma once

#include "rt/callback_timing.h"

#include <cstdint>
#include <fstream>
#include <string>

namespace cli {

// Writes every callback timing to a CSV file (--csv FILE), one row per callback:
//   start_us,duration_us,frames
// Times are in microseconds, starting from the first callback. The plotting
// script reads these files.
//
// Main thread only: file I/O never happens on the audio thread. The timings reach
// this thread through the backend's SPSC queue.
class TimingCsv {
public:
    // Returns false if the file can't be created.
    bool open(const std::string& path, double ticksPerSecond);
    void write(const minerva::CallbackTiming& timing);

private:
    std::ofstream file_;
    double ticksPerSecond_ = 1.0;
    std::int64_t firstStart_ = -1; // -1: no callback written yet
};

} // namespace cli
