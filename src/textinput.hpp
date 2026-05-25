#pragma once

#include <devices/IKeyboard.hpp>

#include <cstddef>
#include <string>

namespace hyprdeck {

    enum class ETextInputAction {
        NONE,
        DELETE_BACKWARD,
        DELETE_FORWARD,
        DELETE_WORD_BACKWARD,
        DELETE_WORD_FORWARD,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_WORD_LEFT,
        MOVE_WORD_RIGHT,
        MOVE_START,
        MOVE_END,
        CLEAR,
        CLEAR_TO_END,
    };

    struct STextInputState {
        std::string text;
        size_t      cursor = 0;

        void        reset();
        void        setText(std::string value);
        bool        apply(ETextInputAction action);
        bool        handleKey(IKeyboard::SKeyEvent event, bool ctrl, bool shift);
    };

    void             resetTextInput(STextInputState& input);
    void             setTextInputText(STextInputState& input, std::string text);
    ETextInputAction textInputActionForKey(IKeyboard::SKeyEvent event, bool ctrl);
    bool             applyTextInputAction(STextInputState& input, ETextInputAction action);
    bool             handleTextInputKey(STextInputState& input, IKeyboard::SKeyEvent event, bool ctrl, bool shift);

} // namespace hyprdeck
