#pragma once

#include "textinput.hpp"

#include <string>
#include <vector>

namespace hyprdeck {

    enum class EShortcutCommand {
        NONE,
        MOVE_SELECTION,
        JUMP_SELECTION,
        SELECT_SPECIAL_ROW,
        SELECT_NORMAL_ROW,
        SWITCH_NORMAL_WORKSPACE,
        TOGGLE_SELECTION,
        OPEN_SELECTION,
        CLOSE_WORKSPACE_WINDOWS,
        CREATE_SIMPLE_SPECIAL,
        CREATE_NAMED_SPECIAL,
        RENAME_SPECIAL,
        ZOOM_PRESET,
        OPEN_WORKSPACE_FILTER,
        CLEAR_WORKSPACE_FILTER,
        PAN_ROW,
        CLOSE_OVERLAY,
        OPEN_KEYBINDINGS,
        CONFIRM_TEXT,
        CANCEL_TEXT,
        DELETE_TEXT_BACKWARD,
        DELETE_TEXT_FORWARD,
        DELETE_TEXT_WORD_BACKWARD,
        DELETE_TEXT_WORD_FORWARD,
        MOVE_TEXT_CURSOR,
        MOVE_TEXT_WORD,
        MOVE_TEXT_LINE_ENDS,
        CLEAR_TEXT,
        TYPE_TEXT,
        SELECT_NAMED_PRESET,
        USE_NAMED_PRESET,
        SEARCH_SHORTCUTS,
    };

    struct SShortcutAction {
        EShortcutCommand command = EShortcutCommand::NONE;
        std::string key;
        std::string label;
        std::string description;
        bool        footer = false;
    };

    std::vector<SShortcutAction> currentShortcutActions();
    std::vector<SShortcutAction> footerShortcutActions();
    EShortcutCommand             shortcutCommandForTextInputAction(ETextInputAction action);

} // namespace hyprdeck
