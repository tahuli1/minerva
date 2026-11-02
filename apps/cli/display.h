#pragma once

#include "controls.h"
#include "rt/timing_stats.h"
#include "rtaudio/rtaudio_backend.h"

#include <cstdint>

namespace cli {

// Everything the CLI prints, laid out as titled sections:
//   AUDIO, KEYS          printed once at startup, values aligned in a column
//   CALLBACK TIMING      a table that grows by one row per report
//   NOW                  the current settings, always the last line
// Main thread only.

void printTitle();
void printAudio(const minerva::StreamInfo& info);
void printKeys();

// The CALLBACK TIMING table. The header is printed again every kRowsPerHeader
// rows, so the column names stay on screen while the table scrolls.
class TimingTable {
public:
    static constexpr int kRowsPerHeader = 20;

    void printTitle();
    void printRow(const minerva::TimingSummary& summary, std::uint64_t droppedTimings);

private:
    void printHeader();
    int rows_ = 0;
};

// Rewrites the NOW line in place (no newline), so it stays the bottom line.
void printSettings(const Settings& settings);

} // namespace cli
