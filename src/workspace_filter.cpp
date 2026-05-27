#include "workspace_filter.hpp"

#include "keyboard.hpp"
#include "layout.hpp"
#include "shortcut_catalog.hpp"
#include "state.hpp"
#include "textinput.hpp"
#include "textinput_repeat.hpp"
#include "workspaces.hpp"

#include <helpers/Monitor.hpp>
#include <render/Renderer.hpp>

#include <linux/input-event-codes.h>
#include <string_view>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        void damageFilter(const PHLMONITOR& monitor) {
            invalidateLayout();
            g_pHyprRenderer->damageMonitor(monitor);
        }

        void focusActiveWorkspace(const PHLMONITOR& monitor) {
            auto& current   = state();
            auto& layout    = current.layout;
            auto& selection = current.selection;

            const auto activeSpecialID = activeSpecialWorkspaceID(monitor);
            const auto activeNormalID  = activeNormalWorkspaceID(monitor);

            layout.resetCamera          = true;
            selection.selectedNormalID  = activeNormalID;
            selection.selectedSpecialID = activeSpecialID;
            selection.selectedRow       = activeSpecialID != WORKSPACE_INVALID ? ESelectedRow::SPECIAL : ESelectedRow::NORMAL;

            invalidateLayout();
            recalculateCards(monitor);

            const int specialIndex = cardIndexByID(layout.specialCards, activeSpecialID);
            if (specialIndex >= 0) {
                selection.selectedRow       = ESelectedRow::SPECIAL;
                selection.selectedSpecialID = activeSpecialID;
                centerSpecialCard(specialIndex);
                return;
            }

            const int normalIndex = cardIndexByID(layout.cards, activeNormalID);
            if (normalIndex >= 0) {
                selection.selectedRow      = ESelectedRow::NORMAL;
                selection.selectedNormalID = activeNormalID;
                centerNormalCard(normalIndex);
                return;
            }

            ensureSelection(monitor);
        }

        STextInputState* workspaceFilterInput() {
            return &state().filter.promptInput;
        }

        bool workspaceFilterRepeatActive() {
            return workspaceFilterPromptOpen();
        }

        void workspaceFilterTextChanged(const PHLMONITOR& monitor) {
            damageFilter(monitor);
        }

        STextInputRepeatTarget workspaceFilterRepeatTarget() {
            return STextInputRepeatTarget{.input = workspaceFilterInput, .active = workspaceFilterRepeatActive, .changed = workspaceFilterTextChanged};
        }

        EShortcutCommand workspaceFilterCommandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_ESC)
                return EShortcutCommand::CANCEL_TEXT;

            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
                return EShortcutCommand::CONFIRM_TEXT;

            const auto command = shortcutCommandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::TYPE_TEXT : command;
        }

        void confirmWorkspaceFilterPrompt(const PHLMONITOR& monitor) {
            auto& filter = state().filter;
            if (!filter.promptOpen)
                return;

            filter.text       = filter.promptInput.text;
            filter.promptOpen = false;
            filter.previousText.clear();
            filter.promptInput.reset();
            stopTextInputRepeat();
            damageFilter(monitor);
        }

    } // namespace

    bool workspaceFilterPromptOpen() {
        return state().filter.promptOpen;
    }

    std::string_view workspaceFilterText() {
        const auto& filter = state().filter;
        return filter.promptOpen ? std::string_view{filter.promptInput.text} : std::string_view{filter.text};
    }

    bool workspaceFilterApplied() {
        return !state().filter.text.empty();
    }

    void openWorkspaceFilterPrompt(const PHLMONITOR& monitor) {
        auto& filter = state().filter;
        stopTextInputRepeat();
        filter.promptOpen   = true;
        filter.previousText = filter.text;
        filter.promptInput.setText(filter.text);
        damageFilter(monitor);
    }

    void closeWorkspaceFilterPrompt(const PHLMONITOR& monitor) {
        auto& filter = state().filter;
        if (!filter.promptOpen)
            return;

        filter.text       = filter.previousText;
        filter.promptOpen = false;
        filter.previousText.clear();
        filter.promptInput.reset();
        stopTextInputRepeat();

        focusActiveWorkspace(monitor);
        g_pHyprRenderer->damageMonitor(monitor);
    }

    void clearWorkspaceFilter(const PHLMONITOR& monitor) {
        auto& filter = state().filter;
        if (!filter.promptOpen && filter.text.empty())
            return;

        filter.promptOpen = false;
        filter.text.clear();
        filter.previousText.clear();
        filter.promptInput.reset();
        stopTextInputRepeat();
        focusActiveWorkspace(monitor);
        damageFilter(monitor);
    }

    void resetWorkspaceFilterPromptState() {
        auto& filter = state().filter;
        if (filter.promptOpen)
            filter.text = filter.previousText;

        filter.promptOpen = false;
        filter.previousText.clear();
        filter.promptInput.reset();
        stopTextInputRepeat();
    }

    void resetWorkspaceFilterState() {
        auto& filter = state().filter;
        filter.promptOpen = false;
        filter.text.clear();
        filter.previousText.clear();
        filter.promptInput.reset();
        stopTextInputRepeat();
    }

    void handleWorkspaceFilterKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!state().filter.promptOpen)
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            stopTextInputRepeatFor(event.keycode);
            return;
        }

        const auto modifiers = keyboardModifiers();
        const auto command   = workspaceFilterCommandForKey(event, modifiers);
        const auto action    = textInputActionForKey(event, modifiers.ctrl);
        bool       dirty     = false;

        switch (command) {
            case EShortcutCommand::CANCEL_TEXT: closeWorkspaceFilterPrompt(monitor); return;
            case EShortcutCommand::CONFIRM_TEXT: confirmWorkspaceFilterPrompt(monitor); return;
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::TYPE_TEXT: {
                auto& filter = state().filter;
                dirty        = filter.promptInput.handleKey(event, modifiers.ctrl, modifiers.shift);

                if (textInputActionRepeats(action))
                    startTextInputRepeat(action, event.keycode, workspaceFilterRepeatTarget());
                break;
            }
            default: return;
        }

        if (dirty)
            workspaceFilterTextChanged(monitor);
    }

} // namespace hyprdeck
