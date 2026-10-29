#pragma once

#include "effects/effect.h"
#include "wav_io.h"

namespace ukefx {

// Runs `effect` over the whole clip in blocks of `blockSize`, exactly as the audio
// callback would, so block-boundary bugs show up offline too. First appends
// `tailSeconds` of silence so echoes ring out instead of being cut off.
// Calls effect.prepare() itself; set parameters before calling this.
void processOffline(Effect& effect, Audio& audio, int blockSize, float tailSeconds);

} // namespace ukefx
