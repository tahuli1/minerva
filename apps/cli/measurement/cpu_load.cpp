#include "measurement/cpu_load.h"

#include <cstddef>
#include <cstdint>
#include <stop_token>

namespace cli {

namespace {

// 32 MB: more than the last-level cache of a desktop CPU, so walking it keeps
// evicting cache lines the audio thread may need.
constexpr std::size_t kBufferBytes = std::size_t{32} * 1024 * 1024;
constexpr std::size_t kCacheLine = 64;

void burn(std::stop_token stop) {
    std::vector<std::uint8_t> buffer(kBufferBytes);
    // volatile: nothing ever reads the buffer, so without it the compiler may drop
    // the writes, and the load with them.
    volatile std::uint8_t* bytes = buffer.data();
    while (!stop.stop_requested())
        for (std::size_t i = 0; i < kBufferBytes; i += kCacheLine) bytes[i] = bytes[i] + 1; // one write per cache line
}

} // namespace

CpuLoad::CpuLoad(int threads) {
    threads_.reserve(static_cast<std::size_t>(threads));
    for (int i = 0; i < threads; ++i) threads_.emplace_back(burn);
}

} // namespace cli
