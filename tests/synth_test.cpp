#include "synth/synth.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBlock = 128;

// Long release, so released notes are still fading when the next note arrives.
void prepare(minerva::Synth& synth, float release = 1.0f) {
    synth.prepare(kSampleRate);
    synth.setWaveform(minerva::Waveform::Saw);
    synth.setEnvelope({0.005f, 0.05f, 0.8f, release});
}

void renderSeconds(minerva::Synth& synth, float seconds) {
    std::vector<float> out(kBlock);
    for (int done = 0; done < static_cast<int>(seconds * kSampleRate); done += kBlock) {
        std::fill(out.begin(), out.end(), 0.0f);
        synth.render(out.data(), kBlock);
    }
}

} // namespace

TEST(Synth, AChordUsesOneVoicePerNote) {
    minerva::Synth synth;
    prepare(synth);
    for (int note : {60, 64, 67}) synth.noteOn(note, 1.0f);
    EXPECT_EQ(synth.activeVoices(), 3);
}

TEST(Synth, TheSameNoteTwiceReusesItsVoice) {
    minerva::Synth synth;
    prepare(synth);
    synth.noteOn(60, 1.0f);
    synth.noteOn(60, 1.0f);
    EXPECT_EQ(synth.activeVoices(), 1);
}

TEST(Synth, NoteOffReleasesOnlyThatNote) {
    minerva::Synth synth;
    prepare(synth, 0.1f);
    synth.noteOn(60, 1.0f);
    synth.noteOn(64, 1.0f);
    synth.noteOff(60);
    renderSeconds(synth, 0.2f); // longer than the release
    EXPECT_FALSE(synth.isSounding(60));
    EXPECT_TRUE(synth.isSounding(64));
}

TEST(Synth, WhenEveryKeyIsHeldTheOldestNoteIsStolen) {
    minerva::Synth synth;
    prepare(synth);
    for (int i = 0; i < minerva::Synth::kVoices; ++i) synth.noteOn(60 + i, 1.0f); // 60..75, all held
    ASSERT_EQ(synth.activeVoices(), minerva::Synth::kVoices);
    synth.noteOn(90, 1.0f);
    EXPECT_EQ(synth.activeVoices(), minerva::Synth::kVoices);
    EXPECT_TRUE(synth.isSounding(90));
    EXPECT_FALSE(synth.isSounding(60)); // the oldest
    EXPECT_TRUE(synth.isSounding(61));
}

TEST(Synth, AReleasedNoteIsStolenBeforeAHeldOne) {
    minerva::Synth synth;
    prepare(synth);
    for (int i = 0; i < minerva::Synth::kVoices; ++i) synth.noteOn(60 + i, 1.0f);
    renderSeconds(synth, 0.1f); // let the notes reach their sustain level
    synth.noteOff(70);          // still fading: the voice is busy, but its key is up
    renderSeconds(synth, 0.05f);
    ASSERT_EQ(synth.activeVoices(), minerva::Synth::kVoices); // otherwise nothing is stolen
    synth.noteOn(90, 1.0f);
    EXPECT_FALSE(synth.isSounding(70)); // the released one was taken...
    EXPECT_TRUE(synth.isSounding(60));  // ...not the oldest held one
}

TEST(Synth, TheQuietestReleasedNoteIsStolenFirst) {
    minerva::Synth synth;
    prepare(synth);
    for (int i = 0; i < minerva::Synth::kVoices; ++i) synth.noteOn(60 + i, 1.0f);
    renderSeconds(synth, 0.1f); // let the notes reach their sustain level
    synth.noteOff(65);
    renderSeconds(synth, 0.4f); // 65 has faded a long way
    synth.noteOff(66);
    renderSeconds(synth, 0.05f); // 66 has only just started fading
    ASSERT_EQ(synth.activeVoices(), minerva::Synth::kVoices); // otherwise nothing is stolen
    synth.noteOn(90, 1.0f);
    EXPECT_FALSE(synth.isSounding(65));
    EXPECT_TRUE(synth.isSounding(66));
}

TEST(Synth, NoNoteIsLeftStuck) {
    // A long, messy sequence of presses and releases, more notes than voices, with
    // repeats and steals. Once every key is up, everything must fall silent within
    // the release time.
    minerva::Synth synth;
    prepare(synth, 0.2f);
    for (int round = 0; round < 50; ++round) {
        const int note = 48 + (round * 7) % 30;
        synth.noteOn(note, 1.0f);
        if (round % 3 == 0) synth.noteOn(note + 4, 0.7f);
        renderSeconds(synth, 0.01f);
        if (round % 2 == 0) synth.noteOff(note);
    }
    for (int note = 0; note < 128; ++note) synth.noteOff(note); // every key up
    renderSeconds(synth, 0.25f);
    EXPECT_EQ(synth.activeVoices(), 0);
}

TEST(Synth, IsSilentWithoutNotes) {
    minerva::Synth synth;
    prepare(synth);
    std::vector<float> out(kBlock, 0.0f);
    synth.render(out.data(), kBlock);
    for (float s : out) EXPECT_EQ(s, 0.0f);
}
