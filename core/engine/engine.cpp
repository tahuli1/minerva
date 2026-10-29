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
        Command{Command::Type::NoteOn, static_cast<std::uint8_t>(note), velocity});
}

bool Engine::noteOff(int note) {
    return commands_.try_push(
        Command{Command::Type::NoteOff, static_cast<std::uint8_t>(note), 0.0f});
}

bool Engine::setWaveform(Waveform waveform) {
    return commands_.try_push(
        Command{Command::Type::SetWaveform, static_cast<std::uint8_t>(waveform), 0.0f});
}

void Engine::apply(const Command& command) {
    switch (command.type) {
    case Command::Type::NoteOn: synth_.noteOn(command.value, command.velocity); break;
    case Command::Type::NoteOff: synth_.noteOff(command.value); break;
    case Command::Type::SetWaveform: synth_.setWaveform(static_cast<Waveform>(command.value)); break;
    }
}

void Engine::process(const float* /*in*/, float* out, int n) {
    for (int i = 0; i < kMaxCommandsPerBlock; ++i) {
        const Command* command = commands_.front();
        if (command == nullptr) break;
        apply(*command);
        commands_.pop();
    }

    std::fill_n(out, n, 0.0f); // the voices add into the buffer, so start from silence
    synth_.render(out, n);
    effects_.process(out, out, n);
}

} // namespace minerva
