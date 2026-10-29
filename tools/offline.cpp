#include "offline.h"

#include <algorithm>

namespace minerva {

void processOffline(Effect& effect, Audio& audio, int blockSize, float tailSeconds) {
    const auto tail = static_cast<std::size_t>(tailSeconds * static_cast<float>(audio.sampleRate));
    audio.samples.resize(audio.samples.size() + tail, 0.0f);

    effect.prepare(audio.sampleRate, blockSize);

    const auto total = audio.samples.size();
    for (std::size_t start = 0; start < total; start += static_cast<std::size_t>(blockSize)) {
        const auto n = std::min(static_cast<std::size_t>(blockSize), total - start);
        float* block = audio.samples.data() + start;
        effect.process(block, block, static_cast<int>(n)); // in place, as every Effect allows
    }
}

} // namespace minerva
