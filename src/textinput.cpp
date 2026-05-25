#include "textinput.hpp"

#include <algorithm>
#include <linux/input-event-codes.h>
#include <utility>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        void clampCursor(STextInputState& input) {
            input.cursor = std::min(input.cursor, input.text.size());
        }

        bool isWordSeparator(const char character) {
            return character == ' ' || character == '\t' || character == '-' || character == '_' || character == ':' || character == '/' || character == '.';
        }

        bool eraseRange(STextInputState& input, const size_t start, const size_t end) {
            if (start >= end || start >= input.text.size())
                return false;

            input.text.erase(start, end - start);
            input.cursor = start;
            return true;
        }

        char inputCharacter(const uint32_t keycode, const bool shift) {
            switch (keycode) {
                case KEY_A: return shift ? 'A' : 'a';
                case KEY_B: return shift ? 'B' : 'b';
                case KEY_C: return shift ? 'C' : 'c';
                case KEY_D: return shift ? 'D' : 'd';
                case KEY_E: return shift ? 'E' : 'e';
                case KEY_F: return shift ? 'F' : 'f';
                case KEY_G: return shift ? 'G' : 'g';
                case KEY_H: return shift ? 'H' : 'h';
                case KEY_I: return shift ? 'I' : 'i';
                case KEY_J: return shift ? 'J' : 'j';
                case KEY_K: return shift ? 'K' : 'k';
                case KEY_L: return shift ? 'L' : 'l';
                case KEY_M: return shift ? 'M' : 'm';
                case KEY_N: return shift ? 'N' : 'n';
                case KEY_O: return shift ? 'O' : 'o';
                case KEY_P: return shift ? 'P' : 'p';
                case KEY_Q: return shift ? 'Q' : 'q';
                case KEY_R: return shift ? 'R' : 'r';
                case KEY_S: return shift ? 'S' : 's';
                case KEY_T: return shift ? 'T' : 't';
                case KEY_U: return shift ? 'U' : 'u';
                case KEY_V: return shift ? 'V' : 'v';
                case KEY_W: return shift ? 'W' : 'w';
                case KEY_X: return shift ? 'X' : 'x';
                case KEY_Y: return shift ? 'Y' : 'y';
                case KEY_Z: return shift ? 'Z' : 'z';
                case KEY_1: return shift ? '!' : '1';
                case KEY_2: return shift ? '@' : '2';
                case KEY_3: return shift ? '#' : '3';
                case KEY_4: return shift ? '$' : '4';
                case KEY_5: return shift ? '%' : '5';
                case KEY_6: return shift ? '^' : '6';
                case KEY_7: return shift ? '&' : '7';
                case KEY_8: return shift ? '*' : '8';
                case KEY_9: return shift ? '(' : '9';
                case KEY_0: return shift ? ')' : '0';
                case KEY_MINUS: return shift ? '_' : '-';
                case KEY_EQUAL: return shift ? '+' : '=';
                case KEY_LEFTBRACE: return shift ? '{' : '[';
                case KEY_RIGHTBRACE: return shift ? '}' : ']';
                case KEY_BACKSLASH: return shift ? '|' : '\\';
                case KEY_SEMICOLON: return shift ? ':' : ';';
                case KEY_APOSTROPHE: return shift ? '"' : '\'';
                case KEY_GRAVE: return shift ? '~' : '`';
                case KEY_COMMA: return shift ? '<' : ',';
                case KEY_DOT: return shift ? '>' : '.';
                case KEY_SLASH: return shift ? '?' : '/';
                case KEY_SPACE: return ' ';
                case KEY_KP0: return '0';
                case KEY_KP1: return '1';
                case KEY_KP2: return '2';
                case KEY_KP3: return '3';
                case KEY_KP4: return '4';
                case KEY_KP5: return '5';
                case KEY_KP6: return '6';
                case KEY_KP7: return '7';
                case KEY_KP8: return '8';
                case KEY_KP9: return '9';
                case KEY_KPDOT: return '.';
                case KEY_KPSLASH: return '/';
                case KEY_KPASTERISK: return '*';
                case KEY_KPMINUS: return '-';
                case KEY_KPPLUS: return '+';
                default: return '\0';
            }
        }

    } // namespace

    void STextInputState::reset() {
        text.clear();
        cursor = 0;
    }

    void STextInputState::setText(std::string value) {
        text   = std::move(value);
        cursor = text.size();
    }

    void resetTextInput(STextInputState& input) {
        input.reset();
    }

    void setTextInputText(STextInputState& input, std::string text) {
        input.setText(std::move(text));
    }

    ETextInputAction textInputActionForKey(const IKeyboard::SKeyEvent event, const bool ctrl) {
        if (event.state != WL_KEYBOARD_KEY_STATE_PRESSED)
            return ETextInputAction::NONE;

        if (ctrl && event.keycode == KEY_W)
            return ETextInputAction::DELETE_WORD_BACKWARD;
        if (ctrl && event.keycode == KEY_U)
            return ETextInputAction::CLEAR;
        if (ctrl && event.keycode == KEY_K)
            return ETextInputAction::CLEAR_TO_END;
        if ((ctrl && event.keycode == KEY_A) || event.keycode == KEY_HOME)
            return ETextInputAction::MOVE_START;
        if ((ctrl && event.keycode == KEY_E) || event.keycode == KEY_END)
            return ETextInputAction::MOVE_END;
        if (event.keycode == KEY_BACKSPACE || (ctrl && event.keycode == KEY_H))
            return ctrl ? ETextInputAction::DELETE_WORD_BACKWARD : ETextInputAction::DELETE_BACKWARD;
        if (event.keycode == KEY_DELETE)
            return ctrl ? ETextInputAction::DELETE_WORD_FORWARD : ETextInputAction::DELETE_FORWARD;
        if (event.keycode == KEY_LEFT)
            return ctrl ? ETextInputAction::MOVE_WORD_LEFT : ETextInputAction::MOVE_LEFT;
        if (event.keycode == KEY_RIGHT)
            return ctrl ? ETextInputAction::MOVE_WORD_RIGHT : ETextInputAction::MOVE_RIGHT;

        return ETextInputAction::NONE;
    }

    bool STextInputState::apply(const ETextInputAction action) {
        clampCursor(*this);

        switch (action) {
            case ETextInputAction::DELETE_BACKWARD:
                if (cursor == 0)
                    return false;
                text.erase(cursor - 1, 1);
                --cursor;
                return true;
            case ETextInputAction::DELETE_FORWARD:
                if (cursor >= text.size())
                    return false;
                text.erase(cursor, 1);
                return true;
            case ETextInputAction::DELETE_WORD_BACKWARD: {
                if (cursor == 0)
                    return false;

                size_t start = cursor;
                while (start > 0 && isWordSeparator(text[start - 1]))
                    --start;
                while (start > 0 && !isWordSeparator(text[start - 1]))
                    --start;

                return eraseRange(*this, start, cursor);
            }
            case ETextInputAction::DELETE_WORD_FORWARD: {
                if (cursor >= text.size())
                    return false;

                size_t end = cursor;
                while (end < text.size() && isWordSeparator(text[end]))
                    ++end;
                while (end < text.size() && !isWordSeparator(text[end]))
                    ++end;

                return eraseRange(*this, cursor, end);
            }
            case ETextInputAction::MOVE_LEFT:
                if (cursor == 0)
                    return false;
                --cursor;
                return true;
            case ETextInputAction::MOVE_RIGHT:
                if (cursor >= text.size())
                    return false;
                ++cursor;
                return true;
            case ETextInputAction::MOVE_WORD_LEFT:
                if (cursor == 0)
                    return false;
                while (cursor > 0 && isWordSeparator(text[cursor - 1]))
                    --cursor;
                while (cursor > 0 && !isWordSeparator(text[cursor - 1]))
                    --cursor;
                return true;
            case ETextInputAction::MOVE_WORD_RIGHT:
                if (cursor >= text.size())
                    return false;
                while (cursor < text.size() && isWordSeparator(text[cursor]))
                    ++cursor;
                while (cursor < text.size() && !isWordSeparator(text[cursor]))
                    ++cursor;
                return true;
            case ETextInputAction::MOVE_START:
                if (cursor == 0)
                    return false;
                cursor = 0;
                return true;
            case ETextInputAction::MOVE_END:
                if (cursor == text.size())
                    return false;
                cursor = text.size();
                return true;
            case ETextInputAction::CLEAR:
                if (text.empty() && cursor == 0)
                    return false;
                reset();
                return true;
            case ETextInputAction::CLEAR_TO_END:
                if (cursor >= text.size())
                    return false;
                text.erase(cursor);
                return true;
            case ETextInputAction::NONE: return false;
        }

        return false;
    }

    bool applyTextInputAction(STextInputState& input, const ETextInputAction action) {
        return input.apply(action);
    }

    bool STextInputState::handleKey(const IKeyboard::SKeyEvent event, const bool ctrl, const bool shift) {
        if (event.state != WL_KEYBOARD_KEY_STATE_PRESSED)
            return false;

        if (const auto action = textInputActionForKey(event, ctrl); action != ETextInputAction::NONE)
            return apply(action);

        if (ctrl)
            return false;

        const char character = inputCharacter(event.keycode, shift);
        if (character == '\0')
            return false;

        clampCursor(*this);
        text.insert(cursor, 1, character);
        ++cursor;
        return true;
    }

    bool handleTextInputKey(STextInputState& input, const IKeyboard::SKeyEvent event, const bool ctrl, const bool shift) {
        return input.handleKey(event, ctrl, shift);
    }

} // namespace hyprdeck
