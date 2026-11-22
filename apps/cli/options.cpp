#include "options.h"

#include <charconv>
#include <cstring>

namespace cli {

namespace {

// A whole number in [min, max], with nothing after it.
bool parseInt(const char* text, int min, int max, int& value) {
    const char* end = text + std::strlen(text);
    int parsed = 0;
    const auto [rest, ec] = std::from_chars(text, end, parsed);
    if (ec != std::errc() || rest != end || parsed < min || parsed > max) return false;
    value = parsed;
    return true;
}

} // namespace

const char* usage() {
    return "Usage: minerva_cli [--stats] [--buffer FRAMES] [--load THREADS] [--csv FILE] "
           "[--seconds N]";
}

bool parseOptions(int argc, const char* const* argv, Options& options, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        const std::string name = argv[i];
        if (name == "--stats") {
            options.showStats = true;
            continue;
        }
        if (name != "--buffer" && name != "--load" && name != "--csv" && name != "--seconds") {
            error = "Unknown option: " + name;
            return false;
        }
        if (i + 1 >= argc) {
            error = name + " needs a value";
            return false;
        }
        const char* value = argv[++i];
        int number = 0;
        if (name == "--csv") {
            options.csvPath = value;
        } else if (name == "--buffer") {
            // 16 frames is 0.33 ms at 48 kHz: smaller than any device offers.
            if (!parseInt(value, 16, 4096, number)) {
                error = "--buffer needs a block size from 16 to 4096 frames";
                return false;
            }
            options.bufferFrames = static_cast<unsigned int>(number);
        } else if (name == "--load") {
            if (!parseInt(value, 0, 64, number)) {
                error = "--load needs a number of threads from 0 to 64";
                return false;
            }
            options.loadThreads = number;
        } else {
            if (!parseInt(value, 1, 24 * 60 * 60, number)) {
                error = "--seconds needs a number of seconds from 1 to 86400";
                return false;
            }
            options.seconds = number;
        }
    }
    return true;
}

} // namespace cli
