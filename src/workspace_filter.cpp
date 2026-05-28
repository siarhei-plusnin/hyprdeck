#include "workspace_filter.hpp"

#include "keyboard.hpp"
#include "layout.hpp"
#include "plugin.hpp"
#include "shortcut_catalog.hpp"
#include "runtime_types.hpp"
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
            activePlugin()->layout().invalidate();
            activePlugin()->hyprland().damageMonitor(monitor);
        }

        void focusActiveWorkspace(const PHLMONITOR& monitor) {
            const auto& workspaces = activePlugin()->workspaces();

            const auto activeSpecialID = workspaces.activeSpecialWorkspaceID(monitor);
            const auto activeNormalID  = workspaces.activeNormalWorkspaceID(monitor);

            activePlugin()->layout().setResetCamera(true);
            activePlugin()->selection().setActiveSelection(activeNormalID, activeSpecialID);

            activePlugin()->layout().invalidate();
            activePlugin()->layout().recalculateCards(monitor);

            const int specialIndex = workspaces.cardIndexByID(activePlugin()->layout().specialCards(), activeSpecialID);
            if (specialIndex >= 0) {
                activePlugin()->selection().setSelectedRow(ESelectedRow::SPECIAL);
                activePlugin()->selection().setSelectedSpecialID(activeSpecialID);
                activePlugin()->layout().centerSpecialCard(specialIndex);
                return;
            }

            const int normalIndex = workspaces.cardIndexByID(activePlugin()->layout().cards(), activeNormalID);
            if (normalIndex >= 0) {
                activePlugin()->selection().setSelectedRow(ESelectedRow::NORMAL);
                activePlugin()->selection().setSelectedNormalID(activeNormalID);
                activePlugin()->layout().centerNormalCard(normalIndex);
                return;
            }

            activePlugin()->selection().ensureSelection(monitor);
        }

        STextInputState* workspaceFilterInput() {
            return activePlugin()->workspaceFilter().promptInput();
        }

        bool workspaceFilterRepeatActive() {
            return activePlugin()->workspaceFilter().promptOpen();
        }

        void workspaceFilterTextChanged(const PHLMONITOR& monitor) {
            activePlugin()->workspaceFilter().handleTextChanged(monitor);
        }

        STextInputRepeatTarget workspaceFilterRepeatTarget() {
            return STextInputRepeatTarget{.input = workspaceFilterInput, .active = workspaceFilterRepeatActive, .changed = workspaceFilterTextChanged};
        }

        EShortcutCommand workspaceFilterCommandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_ESC)
                return EShortcutCommand::CANCEL_TEXT;

            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
                return EShortcutCommand::CONFIRM_TEXT;

            const auto command = activePlugin()->shortcutCatalog().commandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::TYPE_TEXT : command;
        }

    } // namespace

    bool CWorkspaceFilterController::promptOpen() const {
        return m_state.promptOpen;
    }

    std::string_view CWorkspaceFilterController::text() const {
        return m_state.promptOpen ? std::string_view{m_state.promptInput.text} : std::string_view{m_state.text};
    }

    STextInputState* CWorkspaceFilterController::promptInput() {
        return &m_state.promptInput;
    }

    void CWorkspaceFilterController::handleTextChanged(const PHLMONITOR& monitor) {
        damageFilter(monitor);
    }

    void CWorkspaceFilterController::confirmPrompt(const PHLMONITOR& monitor) {
        auto& filter = m_state;
        if (!filter.promptOpen)
            return;

        filter.text       = filter.promptInput.text;
        filter.promptOpen = false;
        filter.previousText.clear();
        filter.promptInput.reset();
        activePlugin()->textInputRepeater().stop();
        damageFilter(monitor);
    }

    bool CWorkspaceFilterController::applied() const {
        return !m_state.text.empty();
    }

    void CWorkspaceFilterController::openPrompt(const PHLMONITOR& monitor) {
        auto& filter = m_state;
        activePlugin()->textInputRepeater().stop();
        filter.promptOpen   = true;
        filter.previousText = filter.text;
        filter.promptInput.setText(filter.text);
        damageFilter(monitor);
    }

    void CWorkspaceFilterController::closePrompt(const PHLMONITOR& monitor) {
        auto& filter = m_state;
        if (!filter.promptOpen)
            return;

        filter.text       = filter.previousText;
        filter.promptOpen = false;
        filter.previousText.clear();
        filter.promptInput.reset();
        activePlugin()->textInputRepeater().stop();

        focusActiveWorkspace(monitor);
        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void CWorkspaceFilterController::clear(const PHLMONITOR& monitor) {
        auto& filter = m_state;
        if (!filter.promptOpen && filter.text.empty())
            return;

        filter.promptOpen = false;
        filter.text.clear();
        filter.previousText.clear();
        filter.promptInput.reset();
        activePlugin()->textInputRepeater().stop();
        focusActiveWorkspace(monitor);
        damageFilter(monitor);
    }

    void CWorkspaceFilterController::resetPromptState() {
        auto& filter = m_state;
        if (filter.promptOpen)
            filter.text = filter.previousText;

        filter.promptOpen = false;
        filter.previousText.clear();
        filter.promptInput.reset();
        activePlugin()->textInputRepeater().stop();
    }

    void CWorkspaceFilterController::resetState() {
        auto& filter = m_state;
        filter.promptOpen = false;
        filter.text.clear();
        filter.previousText.clear();
        filter.promptInput.reset();
        activePlugin()->textInputRepeater().stop();
    }

    void CWorkspaceFilterController::handleKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!m_state.promptOpen)
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            activePlugin()->textInputRepeater().stopFor(event.keycode);
            return;
        }

        const auto modifiers = activePlugin()->hyprland().keyboardModifiers();
        const auto command   = workspaceFilterCommandForKey(event, modifiers);
        const auto action    = textInputActionForKey(event, modifiers.ctrl);
        bool       dirty     = false;

        switch (command) {
            case EShortcutCommand::CANCEL_TEXT: closePrompt(monitor); return;
            case EShortcutCommand::CONFIRM_TEXT: confirmPrompt(monitor); return;
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::TYPE_TEXT: {
                auto& filter = m_state;
                dirty        = filter.promptInput.handleKey(event, modifiers.ctrl, modifiers.shift);

                if (activePlugin()->textInputRepeater().actionRepeats(action))
                    activePlugin()->textInputRepeater().start(action, event.keycode, workspaceFilterRepeatTarget());
                break;
            }
            default: return;
        }

        if (dirty)
            workspaceFilterTextChanged(monitor);
    }

} // namespace hyprdeck
