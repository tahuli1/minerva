#include "synth/voice.h"

#include <gtest/gtest.h>

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

} // namespace

TEST(Voice, NoteNumbersMapToFrequencies) {
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(69), 440.0f);   // A4
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(81), 880.0f);   // an octave up doubles
    EXPECT_FLOAT_EQ(minerva::Voice::noteToHz(57), 220.0f);   // an octave down halves
    EXPECT_NEAR(minerva::Voice::noteToHz(60), 261.63f, 0.01f); // middle C
}

TEST(Voice, PlaysThePitchOfItsNote) {
    auto voice = make(minerva::Waveform::Sine, {0.0f, 0.0f, 1.0f, 0.1f});
    voice.start(69, 1.0f);
    std::vector<float> out(kSampleRate, 0.0f);
    voice.render(out.data(), kSampleRate);
    int crossings = 0;
    for (int i = 1; i < kSampleRate; ++i)
        if (out[i - 1] < 0.0f && out[i] >= 0.0f) ++crossings;
    EXPECT_NEAR(crossings, 440, 1);
}

TEST(Voice, AnIdleVoiceAddsNothing) {
    auto voice = make(minerva::Waveform::Saw, {});
    std::vector<float> out(256, 0.25f);
    voice.render(out.data(), 256);
    for (float s : out) EXPECT_EQ(s, 0.25f);
}

TEST(Voice, RenderAddsInsteadOfOverwriting) {
    // A chord is the sum of its notes, so two voices must be able to share a buffer.
    const minerva::AdsrParams env{0.0f, 0.0f, 1.0f, 0.1f};
    auto a = make(minerva::Waveform::Sine, env), b = make(minerva::Waveform::Sine, env);
    auto a2 = make(minerva::Waveform::Sine, env), b2 = make(minerva::Waveform::Sine, env);
    for (auto* v : {&a, &a2}) v->start(60, 0.5f);
    for (auto* v : {&b, &b2}) v->start(64, 0.5f);

    std::vector<float> together(512, 0.0f), onlyA(512, 0.0f), onlyB(512, 0.0f);
    a.render(together.data(), 512);
    b.render(together.data(), 512);
    a2.render(onlyA.data(), 512);
    b2.render(onlyB.data(), 512);
    for (int i = 0; i < 512; ++i) EXPECT_FLOAT_EQ(together[i], onlyA[i] + onlyB[i]);
}

TEST(Voice, BecomesInactiveWhenTheReleaseEnds) {
    auto voice = make(minerva::Waveform::Saw, {0.01f, 0.05f, 0.7f, 0.2f});
    std::vector<float> out(kSampleRate, 0.0f);
    voice.start(60, 1.0f);
    voice.render(out.data(), kSampleRate / 2);
    EXPECT_TRUE(voice.isActive());
    voice.release();
    voice.render(out.data(), kSampleRate / 10); // 100 ms of a 200 ms release
    EXPECT_TRUE(voice.isActive());
    voice.render(out.data(), kSampleRate / 10); // the other 100 ms
    EXPECT_FALSE(voice.isActive());
    EXPECT_EQ(voice.note(), 60);
}
