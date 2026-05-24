#include "overview_controller.hpp"

#include "constants.hpp"
#include "keyboard.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "navigation.hpp"
#include "overview.hpp"
#include "selection.hpp"
#include "shortcut_catalog.hpp"
#include "state.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        bool zoomInKey(const uint32_t keycode) {
            return keycode == KEY_EQUAL || keycode == KEY_KPPLUS;
        }

        bool zoomOutKey(const uint32_t keycode) {
            return keycode == KEY_MINUS || keycode == KEY_KPMINUS;
        }

        WORKSPACEID normalWorkspaceKey(const uint32_t keycode) {
            switch (keycode) {
                case KEY_1:
                case KEY_KP1: return 1;
                case KEY_2:
                case KEY_KP2: return 2;
                case KEY_3:
                case KEY_KP3: return 3;
                case KEY_4:
                case KEY_KP4: return 4;
                case KEY_5:
                case KEY_KP5: return 5;
                case KEY_6:
                case KEY_KP6: return 6;
                case KEY_7:
                case KEY_KP7: return 7;
                case KEY_8:
                case KEY_KP8: return 8;
                case KEY_9:
                case KEY_KP9: return 9;
                case KEY_0:
                case KEY_KP0: return 10;
                default: return WORKSPACE_INVALID;
            }
        }

        void setPresetZoom(const int direction, const PHLMONITOR& monitor) {
            auto& session = state().session;
            recalculateCards(monitor);

            double target = ZOOM_PRESETS[0];
            if (direction > 0) {
                target = ZOOM_PRESETS[std::size(ZOOM_PRESETS) - 1];
                for (const double preset : ZOOM_PRESETS) {
                    if (preset > session.zoom + 0.001) {
                        target = preset;
                        break;
                    }
                }
            } else {
                for (int i = static_cast<int>(std::size(ZOOM_PRESETS)) - 1; i >= 0; --i) {
                    if (ZOOM_PRESETS[i] < session.zoom - 0.001) {
                        target = ZOOM_PRESETS[i];
                        break;
                    }
                }
            }

            adjustZoom(target / std::max(0.001, session.zoom), monitor);
        }

        EShortcutCommand commandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_ESC)
                return EShortcutCommand::CLOSE_OVERLAY;
            if (modifiers.ctrl && (zoomInKey(event.keycode) || zoomOutKey(event.keycode)))
                return EShortcutCommand::ZOOM_PRESET;
            if (normalWorkspaceKey(event.keycode) != WORKSPACE_INVALID)
                return EShortcutCommand::SWITCH_NORMAL_WORKSPACE;
            if (event.keycode == KEY_H || event.keycode == KEY_LEFT || event.keycode == KEY_L || event.keycode == KEY_RIGHT)
                return modifiers.shift ? EShortcutCommand::JUMP_SELECTION : EShortcutCommand::MOVE_SELECTION;
            if (event.keycode == KEY_SPACE)
                return EShortcutCommand::SPACE_ACTION;
            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
                return EShortcutCommand::OPEN_SELECTION;
            if (event.keycode == KEY_Q)
                return EShortcutCommand::CLOSE_WORKSPACE_WINDOWS;
            if (event.keycode == KEY_A)
                return EShortcutCommand::CREATE_SIMPLE_SPECIAL;
            if (event.keycode == KEY_N)
                return EShortcutCommand::CREATE_NAMED_SPECIAL;
            if (event.keycode == KEY_R)
                return EShortcutCommand::RENAME_SPECIAL;
            if (event.keycode == KEY_J || event.keycode == KEY_DOWN)
                return EShortcutCommand::SELECT_SPECIAL_ROW;
            if (event.keycode == KEY_K || event.keycode == KEY_UP)
                return EShortcutCommand::SELECT_NORMAL_ROW;

            return EShortcutCommand::NONE;
        }

        int directionForKey(const uint32_t keycode) {
            if (keycode == KEY_H || keycode == KEY_LEFT || keycode == KEY_K || keycode == KEY_UP || keycode == KEY_MINUS || keycode == KEY_KPMINUS)
                return -1;

            return 1;
        }

    } // namespace

    bool handleOverviewKeyboardKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (event.state != WL_KEYBOARD_KEY_STATE_PRESSED)
            return false;

        const auto modifiers = keyboardModifiers();

        const auto command = commandForKey(event, modifiers);
        switch (command) {
            case EShortcutCommand::CLOSE_OVERLAY: closeOverview(); return true;
            case EShortcutCommand::ZOOM_PRESET: setPresetZoom(directionForKey(event.keycode), monitor); return true;
            case EShortcutCommand::SWITCH_NORMAL_WORKSPACE: switchNormalWorkspaceByID(normalWorkspaceKey(event.keycode), monitor); return true;
            case EShortcutCommand::MOVE_SELECTION: moveKeyboardSelection(directionForKey(event.keycode), monitor); return true;
            case EShortcutCommand::JUMP_SELECTION: jumpKeyboardSelection(directionForKey(event.keycode), monitor); return true;
            case EShortcutCommand::SPACE_ACTION: spaceKeyboardSelection(monitor); return true;
            case EShortcutCommand::OPEN_SELECTION: openSelectedSpecialAndClose(monitor); return true;
            case EShortcutCommand::CLOSE_WORKSPACE_WINDOWS: closeSelectedWorkspaceWindows(monitor); return true;
            case EShortcutCommand::CREATE_SIMPLE_SPECIAL: createSimpleSpecialWorkspace(monitor); return true;
            case EShortcutCommand::CREATE_NAMED_SPECIAL: openNamedSpecialPrompt(monitor); return true;
            case EShortcutCommand::RENAME_SPECIAL: openRenameSpecialPrompt(monitor); return true;
            case EShortcutCommand::SELECT_SPECIAL_ROW: selectKeyboardRow(ESelectedRow::SPECIAL, monitor); return true;
            case EShortcutCommand::SELECT_NORMAL_ROW: selectKeyboardRow(ESelectedRow::NORMAL, monitor); return true;
            case EShortcutCommand::NONE:
            case EShortcutCommand::PAN_ROW:
            case EShortcutCommand::OPEN_KEYBINDINGS:
            case EShortcutCommand::CONFIRM_TEXT:
            case EShortcutCommand::CANCEL_TEXT:
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::TYPE_TEXT:
            case EShortcutCommand::SELECT_NAMED_PRESET:
            case EShortcutCommand::USE_NAMED_PRESET:
            case EShortcutCommand::SEARCH_SHORTCUTS: return false;
        }

        return false;
    }

} // namespace hyprdeck
