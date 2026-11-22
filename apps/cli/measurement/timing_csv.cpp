#include "measurement/timing_csv.h"

#include <cstdio>

namespace cli {

bool TimingCsv::open(const std::string& path, double ticksPerSecond) {
    ticksPerSecond_ = ticksPerSecond;
    file_.open(path);
    if (!file_) return false;
    file_ << "start_us,duration_us,frames\n";
    return true;
}

void TimingCsv::write(const minerva::CallbackTiming& timing) {
    if (!file_.is_open()) return;
    if (firstStart_ < 0) firstStart_ = timing.startTicks;
    const double toUs = 1e6 / ticksPerSecond_;
    char row[64];
    std::snprintf(row, sizeof row, "%.1f,%.2f,%u\n", static_cast<double>(timing.startTicks - firstStart_) * toUs,
                  static_cast<double>(timing.durationTicks) * toUs, timing.frames);
    file_ << row;
}

} // namespace cli
