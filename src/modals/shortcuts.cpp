#include "shortcuts.hpp"

#include "keyboard.hpp"
#include "plugin.hpp"
#include "shortcut_catalog.hpp"
#include "runtime_types.hpp"
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

            const auto command = activePlugin()->shortcutCatalog().commandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::SEARCH_SHORTCUTS : command;
        }

        STextInputState* shortcutSearchInput() {
            return activePlugin()->shortcuts().searchInput();
        }

        bool shortcutSearchRepeatActive() {
            return activePlugin()->shortcuts().menuOpen();
        }

        void shortcutSearchTextChanged(const PHLMONITOR& monitor) {
            activePlugin()->shortcuts().measure(monitor);
            activePlugin()->hyprland().damageMonitor(monitor);
        }

        STextInputRepeatTarget shortcutSearchRepeatTarget() {
            return STextInputRepeatTarget{.input = shortcutSearchInput, .active = shortcutSearchRepeatActive, .changed = shortcutSearchTextChanged};
        }

    } // namespace

    bool CShortcutMenuController::menuOpen() const {
        return m_state.open;
    }

    bool CShortcutMenuController::isMenuKey(const IKeyboard::SKeyEvent event) const {
        return event.state == WL_KEYBOARD_KEY_STATE_PRESSED && event.keycode == KEY_SLASH && activePlugin()->hyprland().keyboardModifiers().shift;
    }

    void CShortcutMenuController::openMenu(const PHLMONITOR& monitor) {
        auto& shortcuts = m_state;
        shortcuts.open      = true;
        shortcuts.searchInput.reset();
        measure(monitor);
        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void CShortcutMenuController::closeMenu(const PHLMONITOR& monitor) {
        if (!m_state.open)
            return;

        resetState();
        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void CShortcutMenuController::resetState() {
        auto& shortcuts   = m_state;
        shortcuts.open       = false;
        shortcuts.width      = 0.0;
        shortcuts.height     = 0.0;
        shortcuts.keyWidth   = 0.0;
        shortcuts.labelWidth = 0.0;
        shortcuts.descWidth  = 0.0;
        shortcuts.searchInput.reset();
        activePlugin()->textInputRepeater().stop();
    }

    void CShortcutMenuController::handleKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!m_state.open)
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            activePlugin()->textInputRepeater().stopFor(event.keycode);
            return;
        }

        const auto modifiers = activePlugin()->hyprland().keyboardModifiers();
        const auto command   = shortcutMenuCommandForKey(event, modifiers);
        const auto action    = textInputActionForKey(event, modifiers.ctrl);
        bool       dirty     = false;

        switch (command) {
            case EShortcutCommand::CLOSE_OVERLAY: closeMenu(monitor); return;
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::SEARCH_SHORTCUTS: {
                auto& shortcuts = m_state;
                dirty           = shortcuts.searchInput.handleKey(event, modifiers.ctrl, modifiers.shift);

                if (activePlugin()->textInputRepeater().actionRepeats(action))
                    activePlugin()->textInputRepeater().start(action, event.keycode, shortcutSearchRepeatTarget());
                break;
            }
            default: return;
        }

        if (dirty)
            shortcutSearchTextChanged(monitor);
    }

    STextInputState* CShortcutMenuController::searchInput() {
        return &m_state.searchInput;
    }

} // namespace hyprdeck
