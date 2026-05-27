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
#include "workspace_filter.hpp"

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
            if (modifiers.ctrl && event.keycode == KEY_F)
                return EShortcutCommand::OPEN_WORKSPACE_FILTER;
            if ((event.keycode == KEY_DELETE || (modifiers.ctrl && event.keycode == KEY_C)) && workspaceFilterApplied())
                return EShortcutCommand::CLEAR_WORKSPACE_FILTER;
            if (normalWorkspaceKey(event.keycode) != WORKSPACE_INVALID)
                return EShortcutCommand::SWITCH_NORMAL_WORKSPACE;
            if (event.keycode == KEY_H || event.keycode == KEY_LEFT || event.keycode == KEY_L || event.keycode == KEY_RIGHT)
                return modifiers.shift ? EShortcutCommand::JUMP_SELECTION : EShortcutCommand::MOVE_SELECTION;
            if (event.keycode == KEY_SPACE)
                return EShortcutCommand::TOGGLE_SELECTION;
            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER || event.keycode == KEY_F)
                return EShortcutCommand::OPEN_SELECTION;
            if (event.keycode == KEY_Q)
                return EShortcutCommand::CLOSE_WORKSPACE_WINDOWS;
            if (workspaceFilterApplied() && (event.keycode == KEY_A || event.keycode == KEY_N))
                return EShortcutCommand::NONE;
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

        if (modifiers.super) {
            return false;
        }

        const auto command = commandForKey(event, modifiers);
        switch (command) {
            case EShortcutCommand::CLOSE_OVERLAY: closeOverview(); break;

            case EShortcutCommand::ZOOM_PRESET: setPresetZoom(directionForKey(event.keycode), monitor); break;

            case EShortcutCommand::OPEN_WORKSPACE_FILTER: openWorkspaceFilterPrompt(monitor); break;
            case EShortcutCommand::CLEAR_WORKSPACE_FILTER: clearWorkspaceFilter(monitor); break;

            case EShortcutCommand::SWITCH_NORMAL_WORKSPACE: switchNormalWorkspaceByID(normalWorkspaceKey(event.keycode), monitor); break;

            case EShortcutCommand::MOVE_SELECTION: moveSelection(directionForKey(event.keycode), monitor); break;
            case EShortcutCommand::JUMP_SELECTION: jumpSelection(directionForKey(event.keycode), monitor); break;
            case EShortcutCommand::TOGGLE_SELECTION: toggleSelection(monitor); break;
            case EShortcutCommand::OPEN_SELECTION: openSelection(monitor); break;

            case EShortcutCommand::CLOSE_WORKSPACE_WINDOWS: closeSelectedWorkspaceWindows(monitor); break;

            case EShortcutCommand::CREATE_SIMPLE_SPECIAL: createSimpleSpecialWorkspace(monitor); break;
            case EShortcutCommand::CREATE_NAMED_SPECIAL: openNamedSpecialPrompt(monitor); break;

            case EShortcutCommand::RENAME_SPECIAL: openRenameSpecialPrompt(monitor); break;

            case EShortcutCommand::SELECT_SPECIAL_ROW: selectRow(ESelectedRow::SPECIAL, monitor); break;
            case EShortcutCommand::SELECT_NORMAL_ROW: selectRow(ESelectedRow::NORMAL, monitor); break;
            default: break;
        }

        return true;
    }

} // namespace hyprdeck
