#include "note_keys.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using cli::NoteKeys;

namespace {
constexpr int kOctave = NoteKeys::kDefaultOctave;
}

TEST(NoteKeys, TheHomeRowIsTheWhiteKeysFromMiddleC) {
    EXPECT_EQ(NoteKeys::noteFor('A', kOctave), 60); // C
    EXPECT_EQ(NoteKeys::noteFor('S', kOctave), 62); // D
    EXPECT_EQ(NoteKeys::noteFor('F', kOctave), 65); // F: no black key between E and F
    EXPECT_EQ(NoteKeys::noteFor('K', kOctave), 72); // C an octave up
    EXPECT_EQ(NoteKeys::noteFor(NoteKeys::kSemicolonKey, kOctave), 76); // E
}

TEST(NoteKeys, TheRowAboveIsTheBlackKeys) {
    EXPECT_EQ(NoteKeys::noteFor('W', kOctave), 61); // C#
    EXPECT_EQ(NoteKeys::noteFor('T', kOctave), 66); // F#
    EXPECT_EQ(NoteKeys::noteFor('P', kOctave), 75); // D# an octave up
}

TEST(NoteKeys, OtherKeysPlayNothing) {
    EXPECT_FALSE(NoteKeys::noteFor('R', kOctave)); // the gap between E and F
    EXPECT_FALSE(NoteKeys::noteFor('Z', kOctave));
    EXPECT_FALSE(NoteKeys::noteFor('1', kOctave));
}

TEST(NoteKeys, TheOctaveMovesEveryNoteByTwelve) {
    EXPECT_EQ(NoteKeys::noteFor('A', kOctave + 1), 72);
    EXPECT_EQ(NoteKeys::noteFor('A', NoteKeys::kMinOctave), 24);
}

TEST(NoteKeys, AutoRepeatDoesNotRestartAHeldNote) {
    NoteKeys keys;
    EXPECT_EQ(keys.press('A', kOctave), 60);
    EXPECT_FALSE(keys.press('A', kOctave)); // Windows repeats the key-down while held
    EXPECT_FALSE(keys.press('A', kOctave));
    EXPECT_EQ(keys.release('A'), 60);
    EXPECT_EQ(keys.press('A', kOctave), 60); // a new press after the release plays again
}

TEST(NoteKeys, ReleaseStopsTheNoteTheKeyStartedEvenAfterAnOctaveChange) {
    NoteKeys keys;
    keys.press('A', kOctave);
    // The octave goes up while A is held: the repeats are still ignored, and the
    // release must stop 60, not 72, or 60 would be stuck.
    EXPECT_FALSE(keys.press('A', kOctave + 1));
    EXPECT_EQ(keys.release('A'), 60);
}

TEST(NoteKeys, ReleasingAKeyThatIsNotHeldDoesNothing) {
    NoteKeys keys;
    EXPECT_FALSE(keys.release('A'));
    EXPECT_FALSE(keys.press('Z', kOctave));
    EXPECT_FALSE(keys.release('Z'));
}

TEST(NoteKeys, ReleaseAllReturnsEveryHeldNoteOnce) {
    NoteKeys keys;
    keys.press('A', kOctave);
    keys.press('D', kOctave);
    keys.press('G', kOctave);
    keys.release('D');
    std::vector<int> notes = keys.releaseAll();
    std::sort(notes.begin(), notes.end());
    EXPECT_EQ(notes, (std::vector<int>{60, 67}));
    EXPECT_TRUE(keys.releaseAll().empty());
    EXPECT_FALSE(keys.release('A')); // already let go
}

TEST(NoteKeys, KeyCodesOutOfRangeAreIgnored) {
    NoteKeys keys;
    EXPECT_FALSE(keys.press(-1, kOctave));
    EXPECT_FALSE(keys.press(1000, kOctave));
    EXPECT_FALSE(keys.release(1000));
}
