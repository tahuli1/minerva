#include "note_keys.h"

namespace cli {

namespace {

// Semitones above the octave's C. Neighbouring white keys are two semitones apart,
// except E-F and B-C, which have no black key between them; that is why R and I
// play nothing.
struct KeyNote {
    int key;
    int semitone;
};
constexpr KeyNote kKeyNotes[] = {
    {'A', 0},  {'W', 1},  {'S', 2},  {'E', 3},  {'D', 4},  {'F', 5},
    {'T', 6},  {'G', 7},  {'Y', 8},  {'H', 9},  {'U', 10}, {'J', 11},
    {'K', 12}, {'O', 13}, {'L', 14}, {'P', 15}, {NoteKeys::kSemicolonKey, 16},
};

} // namespace

std::optional<int> NoteKeys::noteFor(int key, int octave) {
    for (const KeyNote& k : kKeyNotes)
        if (k.key == key) return (octave + 1) * 12 + k.semitone; // octave 4: A = 60
    return std::nullopt;
}

NoteKeys::NoteKeys() { heldNote_.fill(kNoNote); }

std::optional<int> NoteKeys::press(int key, int octave) {
    if (key < 0 || key >= static_cast<int>(heldNote_.size())) return std::nullopt;
    if (heldNote_[key] != kNoNote) return std::nullopt; // auto-repeat
    const std::optional<int> note = noteFor(key, octave);
    if (note) heldNote_[key] = *note;
    return note;
}

std::optional<int> NoteKeys::release(int key) {
    if (key < 0 || key >= static_cast<int>(heldNote_.size())) return std::nullopt;
    const int note = heldNote_[key];
    if (note == kNoNote) return std::nullopt;
    heldNote_[key] = kNoNote;
    return note;
}

std::vector<int> NoteKeys::releaseAll() {
    std::vector<int> notes;
    for (int& note : heldNote_) {
        if (note == kNoNote) continue;
        notes.push_back(note);
        note = kNoNote;
    }
    return notes;
}

} // namespace cli
