#pragma once

#include <string>

namespace cli {

// What the command line asked for. The defaults are for playing: a 128-frame block
// (2.67 ms at 48 kHz), no measurement output, no artificial load, runs until Esc.
struct Options {
    bool showStats = false;          // --stats: the callback timing table
    unsigned int bufferFrames = 128; // --buffer N: block size the device is asked for
    int loadThreads = 0;             // --load N: threads that keep the CPU busy
    std::string csvPath;             // --csv FILE: every callback's timing, one row each
    int seconds = 0;                 // --seconds N: quit after N seconds (0: run until Esc)
};

// Parses argv into `options`. Returns false and sets `error` on an unknown option,
// a missing value or a value out of range.
bool parseOptions(int argc, const char* const* argv, Options& options, std::string& error);

const char* usage();

} // namespace cli
