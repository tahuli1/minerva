#include "synth/voice.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <new>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;

minerva::Voice make(minerva::Waveform waveform, minerva::AdsrParams envelope) {
    minerva::Voice voice;
    voice.prepare(kSampleRate);
    voice.setWaveform(waveform);
    voice.setEnvelope(envelope);
    return voice;
}

// Counts upward zero crossings: one per cycle of the waveform.
int upwardCrossings(const std::vector<float>& samples) {
    int crossings = 0;
    for (std::size_t i = 1; i < samples.size(); ++i)
        if (samples[i - 1] < 0.0f && samples[i] >= 0.0f) ++crossings;
    return crossings;
}

std::vector<float> render(minerva::Voice& voice, int samples) {
    std::vector<float> out(static_cast<std::size_t>(samples), 0.0f);
    voice.render(out.data(), samples);
    return out;
}

// A flat envelope (instant attack, full sustain) so the output is the waveform itself.
constexpr minerva::AdsrParams kFlat{0.0f, 0.0f, 1.0f, 0.1f};

} // namespace

TEST(Voice, NoteNumbersMapToFrequencies) {
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(69), 440.0f);     // A4
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(81), 880.0f);     // an octave up doubles
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(57), 220.0f);     // an octave down halves
    EXPECT_NEAR(minerva::Voice::noteToHz(60), 261.63f, 0.01f); // middle C
}

TEST(Voice, PlaysThePitchOfItsNote) {
    auto voice = make(minerva::Waveform::Sine, kFlat);
    voice.start(69, 1.0f);
    EXPECT_NEAR(upwardCrossings(render(voice, kSampleRate)), 440, 1); // one second of A4
}

TEST(Voice, StaysInTuneAfterTenMinutes) {
    // The oscillator's phase must wrap every cycle; an ever-growing phase would lose
    // float precision within minutes and the pitch would drift.
    auto voice = make(minerva::Waveform::Sine, kFlat);
    voice.start(69, 1.0f);
    std::vector<float> scratch(kSampleRate, 0.0f);
    for (int second = 0; second < 10 * 60; ++second) voice.render(scratch.data(), kSampleRate);
    EXPECT_NEAR(upwardCrossings(render(voice, kSampleRate)), 440, 1);
}

TEST(Voice, AnIdleVoiceAddsNothing) {
    auto voice = make(minerva::Waveform::Saw, {});
    std::vector<float> out(256, 0.25f);
    voice.render(out.data(), 256);
    for (float s : out) EXPECT_EQ(s, 0.25f);
}

TEST(Voice, RenderAddsInsteadOfOverwriting) {
    // A chord is the sum of its notes, so two voices must be able to share a buffer.
    auto a = make(minerva::Waveform::Sine, kFlat), b = make(minerva::Waveform::Sine, kFlat);
    auto a2 = make(minerva::Waveform::Sine, kFlat), b2 = make(minerva::Waveform::Sine, kFlat);
    for (auto* v : {&a, &a2}) v->start(60, 0.5f);
    for (auto* v : {&b, &b2}) v->start(64, 0.5f);

    std::vector<float> together(512, 0.0f);
    a.render(together.data(), 512);
    b.render(together.data(), 512);
    const auto onlyA = render(a2, 512), onlyB = render(b2, 512);
    for (int i = 0; i < 512; ++i) EXPECT_FLOAT_EQ(together[i], onlyA[i] + onlyB[i]);
}

TEST(Voice, IsSilentWithinTheReleaseTime) {
    // Our release time means "silent by then at the latest", even from full level.
    // DaisySP's own time is a time constant (~4.6x shorter); voice.cpp converts.
    auto voice = make(minerva::Waveform::Saw, {0.0f, 0.0f, 1.0f, 0.2f});
    voice.start(60, 1.0f);
    render(voice, kSampleRate / 2);
    voice.release();
    render(voice, kSampleRate / 10); // 100 ms of a 200 ms release
    EXPECT_TRUE(voice.isActive());
    render(voice, kSampleRate / 10 + 1); // the other 100 ms (+1 sample)
    EXPECT_FALSE(voice.isActive());
    EXPECT_EQ(voice.note(), 60);
}

TEST(Voice, StartingAgainWhileFadingDoesNotJump) {
    // Retriggering continues from the current level instead of dropping to 0 first,
    // which would be an audible click.
    auto voice = make(minerva::Waveform::Sine, {0.005f, 0.05f, 0.8f, 0.5f});
    voice.start(57, 1.0f);
    render(voice, kSampleRate / 4);
    voice.release();
    auto out = render(voice, kSampleRate / 10); // halfway down
    voice.start(57, 1.0f);
    const auto after = render(voice, 64);
    out.insert(out.end(), after.begin(), after.end());
    float largestStep = 0.0f;
    for (std::size_t i = 1; i < out.size(); ++i)
        largestStep = std::max(largestStep, std::abs(out[i] - out[i - 1]));
    // A 220 Hz sine moves at most ~0.03 per sample; a drop to 0 would be far bigger.
    EXPECT_LT(largestStep, 0.05f);
}

TEST(Voice, SawIsAntiAliased) {
    // A naive saw jumps from +1 to -1 in a single sample (a step of ~2). The PolyBLEP
    // saw spreads that jump over two samples, which removes most of the aliasing at
    // high notes, so no single step comes close to 2.
    auto voice = make(minerva::Waveform::Saw, kFlat);
    voice.start(95, 1.0f); // ~1976 Hz
    const auto out = render(voice, kSampleRate / 10);
    float largestStep = 0.0f;
    for (std::size_t i = 1; i < out.size(); ++i)
        largestStep = std::max(largestStep, std::abs(out[i] - out[i - 1]));
    EXPECT_LT(largestStep, 1.6f);
}

namespace {

// Builds a voice in memory pre-filled with `fill`, plays a note and returns the sound.
std::vector<float> playInMemoryFilledWith(unsigned char fill, minerva::Waveform waveform) {
    alignas(minerva::Voice) unsigned char memory[sizeof(minerva::Voice)];
    std::fill(std::begin(memory), std::end(memory), fill);
    auto* voice = new (memory) minerva::Voice;
    voice->prepare(kSampleRate);
    voice->setWaveform(waveform);
    voice->setEnvelope(kFlat);
    voice->start(60, 1.0f);
    std::vector<float> out = render(*voice, 4800);
    voice->~Voice();
    return out;
}

} // namespace

TEST(Voice, SoundsTheSameWhateverItsMemoryHeldBefore) {
    // A voice must not depend on what was in memory before it was built: the
    // engine can live on the stack, where leftovers are anything. All-ones bytes
    // read as a float are NaN. DaisySP's Oscillator once left `last_out_`
    // uninitialized, and the triangle's integrator then started from that NaN and
    // output NaN forever (we patch DaisySP for it).
    for (int w = 0; w < 4; ++w) {
        const auto waveform = static_cast<minerva::Waveform>(w);
        const std::vector<float> fromZeros = playInMemoryFilledWith(0x00, waveform);
        const std::vector<float> fromGarbage = playInMemoryFilledWith(0xFF, waveform);
        EXPECT_TRUE(std::all_of(fromGarbage.begin(), fromGarbage.end(),
                                [](float s) { return std::isfinite(s); }))
            << "waveform " << w;
        EXPECT_EQ(fromGarbage, fromZeros) << "waveform " << w;
    }
}
