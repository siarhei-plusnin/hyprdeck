#include "shortcuts.hpp"

#include "keyboard.hpp"
#include "shortcut_catalog.hpp"
#include "shortcuts_menu.hpp"
#include "state.hpp"
#include "textinput.hpp"
#include "textinput_repeat.hpp"

#include <render/Renderer.hpp>

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        EShortcutCommand shortcutMenuCommandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_ESC || (event.keycode == KEY_SLASH && modifiers.shift))
                return EShortcutCommand::CLOSE_OVERLAY;

            const auto command = shortcutCommandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::SEARCH_SHORTCUTS : command;
        }

        STextInputState* shortcutSearchInput() {
            return &state().shortcuts.searchInput;
        }

        bool shortcutSearchRepeatActive() {
            return shortcutMenuOpen();
        }

        void shortcutSearchTextChanged(const PHLMONITOR& monitor) {
            measureShortcutMenu(monitor);
            g_pHyprRenderer->damageMonitor(monitor);
        }

        STextInputRepeatTarget shortcutSearchRepeatTarget() {
            return STextInputRepeatTarget{.input = shortcutSearchInput, .active = shortcutSearchRepeatActive, .changed = shortcutSearchTextChanged};
        }

    } // namespace

    bool shortcutMenuOpen() {
        return state().shortcuts.open;
    }

    bool isShortcutMenuKey(const IKeyboard::SKeyEvent event) {
        return event.state == WL_KEYBOARD_KEY_STATE_PRESSED && event.keycode == KEY_SLASH && shiftPressed();
    }

    void openShortcutMenu(const PHLMONITOR& monitor) {
        auto& shortcuts = state().shortcuts;
        shortcuts.open      = true;
        shortcuts.searchInput.reset();
        measureShortcutMenu(monitor);
        g_pHyprRenderer->damageMonitor(monitor);
    }

    void closeShortcutMenu(const PHLMONITOR& monitor) {
        if (!state().shortcuts.open)
            return;

        resetShortcutState();
        g_pHyprRenderer->damageMonitor(monitor);
    }

    void resetShortcutState() {
        auto& shortcuts   = state().shortcuts;
        shortcuts.open       = false;
        shortcuts.width      = 0.0;
        shortcuts.height     = 0.0;
        shortcuts.keyWidth   = 0.0;
        shortcuts.labelWidth = 0.0;
        shortcuts.descWidth  = 0.0;
        shortcuts.searchInput.reset();
        stopTextInputRepeat();
    }

    void handleShortcutMenuKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!state().shortcuts.open)
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            stopTextInputRepeatFor(event.keycode);
            return;
        }

        const auto modifiers = keyboardModifiers();
        const auto command   = shortcutMenuCommandForKey(event, modifiers);
        const auto action    = textInputActionForKey(event, modifiers.ctrl);
        bool       dirty     = false;

        switch (command) {
            case EShortcutCommand::CLOSE_OVERLAY: closeShortcutMenu(monitor); return;
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::SEARCH_SHORTCUTS: {
                auto& shortcuts = state().shortcuts;
                dirty           = shortcuts.searchInput.handleKey(event, modifiers.ctrl, modifiers.shift);

                if (textInputActionRepeats(action))
                    startTextInputRepeat(action, event.keycode, shortcutSearchRepeatTarget());
                break;
            }
            default: return;
        }

        if (dirty)
            shortcutSearchTextChanged(monitor);
    }

} // namespace hyprdeck
