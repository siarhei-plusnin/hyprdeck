#pragma once

namespace hyprdeck {

    struct SKeyboardModifiers {
        bool ctrl  = false;
        bool shift = false;
    };

    SKeyboardModifiers keyboardModifiers();
    bool               ctrlPressed();
    bool               shiftPressed();
    int                keyboardRepeatRate();
    int                keyboardRepeatDelay();

} // namespace hyprdeck
