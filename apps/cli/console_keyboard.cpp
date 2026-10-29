#include "console_keyboard.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <iterator>

namespace cli {

std::vector<KeyEvent> ConsoleKeyboard::wait(unsigned long timeoutMs) {
    std::vector<KeyEvent> events;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    // The console's input handle is signaled while its buffer holds events, so the
    // thread sleeps here instead of polling.
    if (WaitForSingleObject(input, timeoutMs) != WAIT_OBJECT_0) return events;

    INPUT_RECORD records[64];
    DWORD count = 0;
    if (!ReadConsoleInputW(input, records, static_cast<DWORD>(std::size(records)), &count))
        return events;

    for (DWORD i = 0; i < count; ++i) {
        const INPUT_RECORD& r = records[i];
        if (r.EventType == KEY_EVENT) {
            const KEY_EVENT_RECORD& k = r.Event.KeyEvent;
            const bool ctrl = (k.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
            events.push_back({k.bKeyDown ? KeyEvent::Type::Down : KeyEvent::Type::Up,
                              k.wVirtualKeyCode, ctrl});
        } else if (r.EventType == FOCUS_EVENT && !r.Event.FocusEvent.bSetFocus) {
            events.push_back({KeyEvent::Type::FocusLost});
        }
    }
    return events;
}

} // namespace cli
