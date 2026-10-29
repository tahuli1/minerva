#include "engine/engine.h"

#include <algorithm>

namespace minerva {

Engine::Engine() : commands_(kQueueCapacity) {
    effects_.add(distortion_);
    effects_.add(delay_);
}

void Engine::prepare(int sampleRate, int maxBlockSize) {
    synth_.prepare(sampleRate);
    effects_.prepare(sampleRate, maxBlockSize);
}

bool Engine::noteOn(int note, float velocity) {
    return commands_.try_push(
        NoteCommand{NoteCommand::Type::NoteOn, static_cast<std::uint8_t>(note), velocity});
}

bool Engine::noteOff(int note) {
    return commands_.try_push(
        NoteCommand{NoteCommand::Type::NoteOff, static_cast<std::uint8_t>(note), 0.0f});
}

void Engine::process(const float* /*in*/, float* out, int n) {
    for (int i = 0; i < kMaxCommandsPerBlock; ++i) {
        const NoteCommand* command = commands_.front();
        if (command == nullptr) break;
        if (command->type == NoteCommand::Type::NoteOn) synth_.noteOn(command->note, command->velocity);
        else synth_.noteOff(command->note);
        commands_.pop();
    }

    std::fill_n(out, n, 0.0f); // the voices add into the buffer, so start from silence
    synth_.render(out, n);
    effects_.process(out, out, n);
}

} // namespace minerva
