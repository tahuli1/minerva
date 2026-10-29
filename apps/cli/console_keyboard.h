#pragma once

#include <vector>

namespace cli {

struct KeyEvent {
    enum class Type { Down, Up, FocusLost };
    Type type;
    int key = 0; // Windows virtual key code; 0 for FocusLost
    bool ctrl = false;
};

// Reads the keyboard through the console's input buffer (ReadConsoleInput), which,
// unlike _getch(), reports key releases and names keys by virtual key code. Windows
// fills the buffer as keys are pressed, so nothing is missed between reads.
//
// Main thread only, never the audio thread: waiting for a key is the whole point.
class ConsoleKeyboard {
public:
    // Sleeps until there is input or `timeoutMs` passes (so the caller can check
    // other things, like a Ctrl+C), then returns every event waiting, in order.
    // Several keys pressed together come back from one call, in one batch.
    std::vector<KeyEvent> wait(unsigned long timeoutMs);
};

} // namespace cli
