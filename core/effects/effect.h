#pragma once

namespace minerva {

// Base class for every effect in the chain. Mono, float samples in [-1, 1].
class Effect {
public:
    // Not RT: called before the stream starts. Allocate buffers and compute
    // sample-rate dependent constants here, never in process().
    virtual void prepare(int sampleRate, int maxBlockSize) = 0;

    // RT: runs on the audio thread. No allocation, locks, I/O or exceptions.
    // n <= maxBlockSize. in and out may point to the same buffer (in-place).
    virtual void process(const float* in, float* out, int n) = 0;

    virtual ~Effect() = default;
};

} // namespace minerva
