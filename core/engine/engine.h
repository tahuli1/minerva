#pragma once

#include "effects/delay/delay.h"
#include "effects/distortion/distortion.h"
#include "effects/effect_chain.h"
#include "synth/synth.h"

#include <rigtorp/SPSCQueue.h>

#include <cstddef>
#include <cstdint>

namespace minerva {

// A note event on its way from the control thread to the audio thread. Events, not
// values: a key press must never be lost or merged with another, which an atomic
// holding "the latest note" could do.
struct NoteCommand {
    enum class Type : std::uint8_t { NoteOn, NoteOff };
    Type type;
    std::uint8_t note;
    float velocity;
};

// Owns everything that makes sound: the synth, the master effect chain
// (distortion -> delay) and the queue that brings note commands in.
//
// Threads: exactly one control thread calls noteOn()/noteOff() (the queue's single
// producer), and only the audio thread calls process() (its single consumer).
class Engine {
public:
    static constexpr std::size_t kQueueCapacity = 256;
    // A bound on the work per block: commands beyond this wait for the next block.
    static constexpr int kMaxCommandsPerBlock = 64;

    Engine(); // allocates the queue: create the engine before audio starts
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Not RT: call before audio starts.
    void prepare(int sampleRate, int maxBlockSize);

    // Control thread. Never blocks: returns false if the queue is full.
    bool noteOn(int note, float velocity);
    bool noteOff(int note);

    // Audio thread. `in` (the mic) is not used yet; it is for the looper (Milestone 8).
    void process(const float* in, float* out, int n);

    Synth& synth() { return synth_; }
    Distortion& distortion() { return distortion_; }
    Delay& delay() { return delay_; }
    std::size_t pendingCommands() const { return commands_.size(); }

private:
    rigtorp::SPSCQueue<NoteCommand> commands_;
    Synth synth_;
    Distortion distortion_;
    Delay delay_;
    EffectChain effects_;
};

} // namespace minerva
