#pragma once

#include <array>
#include <optional>
#include <vector>

namespace cli {

// Turns computer keys into notes, like the "musical typing" keyboards of music apps:
// the middle letter row is the white keys (A = C ... ; = E an octave up) and the
// row above it the black keys (W = C# ... P = D#).
//
// Keys are Windows virtual key codes, which name the physical key whatever the
// keyboard language is ('A' is the A key even when typing Hebrew). They are plain
// numbers, so this class needs no Windows header and is tested without a console.
//
// Main thread only.
class NoteKeys {
public:
    static constexpr int kMinOctave = 1;
    static constexpr int kMaxOctave = 7;
    static constexpr int kDefaultOctave = 4;   // A plays middle C (note 60)
    static constexpr int kSemicolonKey = 0xBA; // VK_OEM_1, the ; key on a US layout

    // The note `key` plays in `octave`, or nothing if it is not a note key.
    static std::optional<int> noteFor(int key, int octave);

    NoteKeys();

    // A key went down. Returns the note to start, or nothing if it is not a note key
    // or is already down: holding a key makes Windows repeat the key-down event
    // (auto-repeat), and a held note must not restart over and over.
    std::optional<int> press(int key, int octave);
    // A key went up. Returns the note to stop: the one this key started, even if the
    // octave changed in between, or the old note would never stop.
    std::optional<int> release(int key);
    // Forgets every held key and returns their notes, for when the window loses
    // focus and the key-up events will go to another window.
    std::vector<int> releaseAll();

private:
    static constexpr int kNoNote = -1;
    std::array<int, 256> heldNote_; // by virtual key code: the note it started
};

} // namespace cli
