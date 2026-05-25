#pragma once

namespace hyprdeck {

    struct SKeyboardModifiers {
        bool ctrl  = false;
        bool shift = false;
        bool super = false;
    };

    SKeyboardModifiers keyboardModifiers();
    bool               ctrlPressed();
    bool               shiftPressed();
    int                keyboardRepeatRate();
    int                keyboardRepeatDelay();

} // namespace hyprdeck
