#include "shortcut_catalog.hpp"

#include "confirmation.hpp"
#include "config.hpp"
#include "state.hpp"
#include "workspace_filter.hpp"

namespace hyprdeck {
    namespace {

        std::vector<SShortcutAction> overviewActions() {
            const auto&                  selection = state().selection;
            std::vector<SShortcutAction> actions;

            actions.push_back({.command = EShortcutCommand::MOVE_SELECTION, .key = "h/l, left/right", .label = "Move", .description = "Move selection across the current row", .footer = true});

            if (selection.selectedRow == ESelectedRow::NORMAL) {
                actions.push_back({.command = EShortcutCommand::SELECT_SPECIAL_ROW, .key = "j, down", .label = "Special row", .description = "Move focus to the special workspace row", .footer = true});
                actions.push_back({.command = EShortcutCommand::SWITCH_NORMAL_WORKSPACE, .key = "1-0", .label = "Switch", .description = "Switch to normal workspace 1-10", .footer = true});
                actions.push_back({.command = EShortcutCommand::SPACE_ACTION, .key = "space", .label = "Hide special", .description = "Close the active special workspace without leaving overview", .footer = false});
                actions.push_back({.command = EShortcutCommand::OPEN_SELECTION, .key = "enter", .label = "Open normal", .description = "Close overview on the selected normal workspace", .footer = false});
                actions.push_back({.command = EShortcutCommand::CLOSE_WORKSPACE_WINDOWS, .key = "q", .label = "Close apps", .description = "Ask all windows on the selected normal workspace to close", .footer = true});
            } else {
                actions.push_back({.command = EShortcutCommand::SELECT_NORMAL_ROW, .key = "k, up", .label = "Normal row", .description = "Move focus to the normal workspace row", .footer = true});
                actions.push_back({.command = EShortcutCommand::SPACE_ACTION, .key = "space", .label = "Toggle special", .description = "Open or hide the selected special workspace", .footer = false});
                actions.push_back({.command = EShortcutCommand::OPEN_SELECTION, .key = "enter", .label = "Open special", .description = "Open the selected special workspace and close overview", .footer = true});
                actions.push_back({.command = EShortcutCommand::RENAME_SPECIAL, .key = "r", .label = "Rename", .description = "Rename the selected special workspace", .footer = true});
                actions.push_back({.command = EShortcutCommand::CLOSE_WORKSPACE_WINDOWS, .key = "q", .label = "Close apps", .description = "Ask all windows on the selected special workspace to close", .footer = true});
            }

            if (!workspaceFilterApplied()) {
                actions.push_back({.command = EShortcutCommand::CREATE_SIMPLE_SPECIAL, .key = "a", .label = "New special", .description = "Create an unnamed special workspace", .footer = true});
                actions.push_back({.command = EShortcutCommand::CREATE_NAMED_SPECIAL, .key = "n", .label = "Named special", .description = "Create a preset or dynamically named special workspace", .footer = true});
            }

            actions.push_back({.command = EShortcutCommand::JUMP_SELECTION, .key = "shift+h/l", .label = "Jump", .description = "Jump to the first or last card in the current row", .footer = false});
            actions.push_back({.command = EShortcutCommand::ZOOM_PRESET, .key = "ctrl+= / ctrl+-", .label = "Zoom", .description = "Cycle workspace preview zoom presets", .footer = false});
            actions.push_back({.command = EShortcutCommand::OPEN_WORKSPACE_FILTER, .key = "ctrl+f", .label = "Filter", .description = "Filter workspaces by window class or title", .footer = true});

            actions.push_back({.command = EShortcutCommand::CLEAR_WORKSPACE_FILTER,
                               .key = "delete / ctrl+c",
                               .label = "Clear filter",
                               .description = "Show all workspaces again when a filter is active",
                               .footer = workspaceFilterApplied()});

            actions.push_back({.command = EShortcutCommand::PAN_ROW, .key = "scroll / drag", .label = "Pan", .description = "Move the row under the pointer", .footer = false});
            actions.push_back({.command = EShortcutCommand::CLOSE_OVERLAY, .key = "esc / right click", .label = "Close", .description = "Close the overview", .footer = false});
            actions.push_back({.command = EShortcutCommand::OPEN_KEYBINDINGS, .key = "?", .label = "Keybindings", .description = "Open this searchable shortcuts menu", .footer = true});
            return actions;
        }

        std::vector<SShortcutAction> workspaceFilterActions() {
            return {
                {.command = EShortcutCommand::CONFIRM_TEXT, .key = "enter", .label = "Apply", .description = "Keep this workspace filter active", .footer = true},
                {.command = EShortcutCommand::CANCEL_TEXT, .key = "esc", .label = "Cancel", .description = "Close the filter field without applying edits", .footer = true},
                {.command = EShortcutCommand::DELETE_TEXT_BACKWARD, .key = "backspace", .label = "Delete", .description = "Edit the filter text; hold to repeat", .footer = true},
                {.command = EShortcutCommand::DELETE_TEXT_FORWARD, .key = "delete", .label = "Delete forward", .description = "Edit the filter text; hold to repeat", .footer = false},
                {.command = EShortcutCommand::MOVE_TEXT_CURSOR, .key = "left/right", .label = "Move cursor", .description = "Move inside the filter text", .footer = true},
                {.command = EShortcutCommand::MOVE_TEXT_WORD, .key = "ctrl+left/right", .label = "Move word", .description = "Move the cursor by word", .footer = false},
                {.command = EShortcutCommand::CLEAR_TEXT, .key = "ctrl+u / ctrl+k", .label = "Clear", .description = "Clear the filter field", .footer = false},
                {.command = EShortcutCommand::TYPE_TEXT, .key = "type", .label = "Type", .description = "Filter by window class or title", .footer = false},
                {.command = EShortcutCommand::OPEN_KEYBINDINGS, .key = "?", .label = "Keybindings", .description = "Open this searchable shortcuts menu", .footer = true},
            };
        }

        std::vector<SShortcutAction> confirmationActions() {
            return {
                {.command = EShortcutCommand::CONFIRM_TEXT, .key = "enter", .label = "Close", .description = "Close all windows on the selected normal workspace", .footer = true},
                {.command = EShortcutCommand::CANCEL_TEXT, .key = "esc", .label = "Cancel", .description = "Keep the workspace windows open", .footer = true},
                {.command = EShortcutCommand::OPEN_KEYBINDINGS, .key = "?", .label = "Keybindings", .description = "Open this searchable shortcuts menu", .footer = true},
            };
        }

        std::vector<SShortcutAction> namingActions() {
            const auto&                  naming = state().naming;
            std::vector<SShortcutAction> actions;

            if (naming.promptMode == EPromptMode::RENAME_SPECIAL) {
                actions.push_back({.command = EShortcutCommand::CONFIRM_TEXT, .key = "enter", .label = "Confirm", .description = "Rename the selected special workspace", .footer = true});
            } else {
                actions.push_back({.command = EShortcutCommand::CONFIRM_TEXT, .key = "enter", .label = "Confirm", .description = "Create or open the selected special workspace name", .footer = true});
            }

            actions.push_back({.command = EShortcutCommand::CANCEL_TEXT, .key = "esc", .label = "Cancel", .description = "Close the naming prompt", .footer = true});
            actions.push_back({.command = EShortcutCommand::DELETE_TEXT_BACKWARD, .key = "backspace", .label = "Delete", .description = "Delete the character before the cursor; hold to repeat", .footer = true});
            actions.push_back({.command = EShortcutCommand::DELETE_TEXT_WORD_BACKWARD, .key = "ctrl+w", .label = "Delete word", .description = "Delete the word before the cursor", .footer = true});
            actions.push_back({.command = EShortcutCommand::MOVE_TEXT_CURSOR, .key = "left/right", .label = "Move cursor", .description = "Move inside the typed name", .footer = true});
            actions.push_back({.command = EShortcutCommand::MOVE_TEXT_WORD, .key = "ctrl+left/right", .label = "Move word", .description = "Move the cursor by word", .footer = false});
            actions.push_back({.command = EShortcutCommand::DELETE_TEXT_FORWARD, .key = "delete", .label = "Delete forward", .description = "Delete the character after the cursor; hold to repeat", .footer = false});
            actions.push_back({.command = EShortcutCommand::DELETE_TEXT_WORD_FORWARD, .key = "ctrl+delete", .label = "Delete next word", .description = "Delete the word after the cursor", .footer = false});
            actions.push_back({.command = EShortcutCommand::MOVE_TEXT_LINE_ENDS, .key = "ctrl+a / ctrl+e", .label = "Line ends", .description = "Move to the beginning or end of the name", .footer = false});
            actions.push_back({.command = EShortcutCommand::CLEAR_TEXT, .key = "ctrl+u / ctrl+k", .label = "Clear", .description = "Clear all text or clear from cursor to end", .footer = false});
            actions.push_back({.command = EShortcutCommand::TYPE_TEXT, .key = "type", .label = "Type", .description = "Edit the workspace name", .footer = false});

            if (naming.promptMode == EPromptMode::CREATE_SPECIAL && !configuredSpecialWorkspaceNames().empty()) {
                actions.push_back({.command = EShortcutCommand::SELECT_NAMED_PRESET, .key = "up/down", .label = "Select preset", .description = "Move through preset special workspace names", .footer = true});
                actions.push_back({.command = EShortcutCommand::SELECT_NAMED_PRESET, .key = "ctrl+p / ctrl+n", .label = "Select preset", .description = "Move previous or next through preset names", .footer = true});
                actions.push_back({.command = EShortcutCommand::USE_NAMED_PRESET, .key = "space", .label = "Use preset", .description = "When no custom text is typed, create/open the selected preset", .footer = false});
            }

            actions.push_back({.command = EShortcutCommand::OPEN_KEYBINDINGS, .key = "?", .label = "Keybindings", .description = "Open this searchable shortcuts menu", .footer = true});
            return actions;
        }

    } // namespace

    std::vector<SShortcutAction> currentShortcutActions() {
        if (workspaceFilterPromptOpen())
            return workspaceFilterActions();

        if (confirmationPromptOpen())
            return confirmationActions();

        if (state().naming.promptMode != EPromptMode::NONE)
            return namingActions();

        return overviewActions();
    }

    std::vector<SShortcutAction> footerShortcutActions() {
        switch (configuredShortcutsFooterMode()) {
            case EShortcutsFooterMode::NONE: return {};
            case EShortcutsFooterMode::HINT:
                return {{.command = EShortcutCommand::OPEN_KEYBINDINGS, .key = "?", .label = "Keybindings", .description = "Open searchable shortcuts menu", .footer = true}};
            case EShortcutsFooterMode::FULL: break;
        }

        if (state().shortcuts.open)
            return {
                {.command = EShortcutCommand::SEARCH_SHORTCUTS, .key = "type", .label = "Search", .description = "Filter keybindings", .footer = true},
                {.command = EShortcutCommand::DELETE_TEXT_BACKWARD, .key = "backspace", .label = "Delete", .description = "Edit search text", .footer = true},
                {.command = EShortcutCommand::DELETE_TEXT_FORWARD, .key = "delete", .label = "Delete forward", .description = "Edit search text", .footer = true},
                {.command = EShortcutCommand::CLEAR_TEXT, .key = "ctrl+u", .label = "Clear", .description = "Clear the search", .footer = true},
                {.command = EShortcutCommand::CLOSE_OVERLAY, .key = "esc / ?", .label = "Close", .description = "Close keybindings", .footer = true},
            };

        auto actions = currentShortcutActions();
        std::erase_if(actions, [](const auto& action) { return !action.footer || action.command == EShortcutCommand::PAN_ROW; });
        return actions;
    }

    EShortcutCommand shortcutCommandForTextInputAction(const ETextInputAction action) {
        switch (action) {
            case ETextInputAction::DELETE_BACKWARD: return EShortcutCommand::DELETE_TEXT_BACKWARD;
            case ETextInputAction::DELETE_FORWARD: return EShortcutCommand::DELETE_TEXT_FORWARD;
            case ETextInputAction::DELETE_WORD_BACKWARD: return EShortcutCommand::DELETE_TEXT_WORD_BACKWARD;
            case ETextInputAction::DELETE_WORD_FORWARD: return EShortcutCommand::DELETE_TEXT_WORD_FORWARD;
            case ETextInputAction::MOVE_LEFT:
            case ETextInputAction::MOVE_RIGHT: return EShortcutCommand::MOVE_TEXT_CURSOR;
            case ETextInputAction::MOVE_WORD_LEFT:
            case ETextInputAction::MOVE_WORD_RIGHT: return EShortcutCommand::MOVE_TEXT_WORD;
            case ETextInputAction::MOVE_START:
            case ETextInputAction::MOVE_END: return EShortcutCommand::MOVE_TEXT_LINE_ENDS;
            case ETextInputAction::CLEAR:
            case ETextInputAction::CLEAR_TO_END: return EShortcutCommand::CLEAR_TEXT;
            case ETextInputAction::NONE: return EShortcutCommand::NONE;
        }

        return EShortcutCommand::NONE;
    }

} // namespace hyprdeck
