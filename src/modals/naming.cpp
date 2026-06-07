#include "naming.hpp"

#include "config.hpp"
#include "keyboard.hpp"
#include "layout.hpp"
#include "navigation.hpp"
#include "plugin.hpp"
#include "selection.hpp"
#include "shortcut_catalog.hpp"
#include "runtime_types.hpp"
#include "strings.hpp"
#include "textinput.hpp"
#include "textinput_repeat.hpp"

#include <helpers/Monitor.hpp>
#include <render/Renderer.hpp>

#include <algorithm>
#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        void damagePrompt(const PHLMONITOR& monitor) {
            activePlugin()->hyprland().damageMonitor(monitor);
        }

        int presetSelectionDirection(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_UP || (modifiers.ctrl && event.keycode == KEY_P))
                return -1;

            return 1;
        }

        bool presetSelectionLoops(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            return event.keycode == KEY_UP || event.keycode == KEY_DOWN || (modifiers.ctrl && (event.keycode == KEY_N || event.keycode == KEY_P));
        }

        EShortcutCommand namingCommandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers, const EPromptMode promptMode, const bool promptEmpty) {
            if (event.keycode == KEY_ESC)
                return EShortcutCommand::CANCEL_TEXT;

            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
                return EShortcutCommand::CONFIRM_TEXT;

            if (promptMode == EPromptMode::CREATE_SPECIAL &&
                (event.keycode == KEY_UP || event.keycode == KEY_DOWN || (modifiers.ctrl && event.keycode == KEY_P) || (modifiers.ctrl && event.keycode == KEY_N)))
                return EShortcutCommand::SELECT_NAMED_PRESET;

            if (event.keycode == KEY_SPACE && promptMode == EPromptMode::CREATE_SPECIAL && promptEmpty)
                return EShortcutCommand::USE_NAMED_PRESET;

            const auto command = activePlugin()->shortcutCatalog().commandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::TYPE_TEXT : command;
        }

        STextInputState* namingPromptInput() {
            return activePlugin()->naming().promptInput();
        }

        bool namingRepeatActive() {
            return activePlugin()->naming().promptOpen();
        }

        void namingTextChanged(const PHLMONITOR& monitor) {
            activePlugin()->naming().handleTextChanged(monitor);
        }

        STextInputRepeatTarget namingRepeatTarget() {
            return STextInputRepeatTarget{.input = namingPromptInput, .active = namingRepeatActive, .changed = namingTextChanged};
        }

    } // namespace

    bool CNamingController::promptOpen() const {
        return m_state.promptMode != EPromptMode::NONE;
    }

    EPromptMode CNamingController::promptMode() const {
        return m_state.promptMode;
    }

    STextInputState* CNamingController::promptInput() {
        return &m_state.promptInput;
    }

    std::vector<std::string> CNamingController::filteredPresetNames() const {
        auto names = activePlugin()->config().specialWorkspaceNames();
        if (m_state.promptMode != EPromptMode::CREATE_SPECIAL || m_state.promptInput.text.empty())
            return names;

        const auto query = strings::normalizeSpecialWorkspaceName(m_state.promptInput.text);
        if (query.empty())
            return names;

        std::erase_if(names, [&](const auto& name) { return !strings::containsInsensitive(name, query); });
        return names;
    }

    void CNamingController::syncCustomSelectionAfterEdit() {
        auto& naming = m_state;
        if (naming.promptMode == EPromptMode::CREATE_SPECIAL) {
            naming.promptCustomSelected = !naming.promptInput.text.empty();

            const auto names               = filteredPresetNames();
            naming.namedSpecialPromptIndex = names.empty() || naming.promptCustomSelected ? 0 : std::min(naming.namedSpecialPromptIndex, names.size() - 1);
        }
    }

    void CNamingController::handleTextChanged(const PHLMONITOR& monitor) {
        syncCustomSelectionAfterEdit();
        damagePrompt(monitor);
    }

    bool CNamingController::movePresetSelection(const int direction, const bool loop) {
        auto& naming = m_state;
        if (naming.promptMode != EPromptMode::CREATE_SPECIAL)
            return false;

        const auto names         = filteredPresetNames();
        const bool hasCustomName = !naming.promptInput.text.empty();
        if (names.empty()) {
            if (hasCustomName && !naming.promptCustomSelected) {
                naming.promptCustomSelected = true;
                return true;
            }

            return false;
        }

        if (hasCustomName && naming.promptCustomSelected) {
            naming.promptCustomSelected    = false;
            naming.namedSpecialPromptIndex = direction < 0 ? names.size() - 1 : 0;
            return true;
        }

        if (loop) {
            const size_t currentIndex = std::min(naming.namedSpecialPromptIndex, names.size() - 1);
            const size_t nextIndex    = direction < 0 ? (currentIndex == 0 ? names.size() - 1 : currentIndex - 1) : (currentIndex + 1) % names.size();
            if (nextIndex == currentIndex)
                return false;

            naming.promptCustomSelected    = false;
            naming.namedSpecialPromptIndex = nextIndex;
            return true;
        }

        if (hasCustomName && direction < 0 && naming.namedSpecialPromptIndex == 0) {
            naming.promptCustomSelected = true;
            return true;
        }

        const int index = std::clamp(static_cast<int>(naming.namedSpecialPromptIndex) + direction, 0, static_cast<int>(names.size()) - 1);
        if (index == static_cast<int>(naming.namedSpecialPromptIndex))
            return false;

        naming.promptCustomSelected    = false;
        naming.namedSpecialPromptIndex = static_cast<size_t>(index);
        return true;
    }

    bool CNamingController::useSelectedPreset(const PHLMONITOR& monitor) {
        const auto names = filteredPresetNames();
        if (names.empty())
            return false;

        auto& naming                   = m_state;
        naming.namedSpecialPromptIndex = std::min(naming.namedSpecialPromptIndex, names.size() - 1);
        return applySpecialNavigationResult(activePlugin()->navigator().createNamedSpecialWorkspace(names[naming.namedSpecialPromptIndex], monitor), monitor, true);
    }

    bool CNamingController::applySpecialNavigationResult(const SWorkspaceNavigationResult& result, const PHLMONITOR& monitor, const bool centerSpecial) {
        if (!activePlugin()->selection().applyNavigationResult(result))
            return false;

        resetPromptState();
        activePlugin()->layout().invalidate();
        activePlugin()->layout().recalculateCards(monitor);

        if (centerSpecial && result.selectedSpecialID) {
            activePlugin()->layout().centerSpecialCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), *result.selectedSpecialID));
            activePlugin()->animations().startSpecialCardAppearance(*result.selectedSpecialID, monitor);
        }

        damagePrompt(monitor);
        return true;
    }

    void CNamingController::confirmPrompt(const PHLMONITOR& monitor) {
        auto& naming = m_state;
        if (naming.promptMode == EPromptMode::RENAME_SPECIAL) {
            const auto* card = activePlugin()->selection().selectedSpecialCard();
            if (card)
                applySpecialNavigationResult(activePlugin()->navigator().renameSpecialWorkspace(card->workspace, naming.promptInput.text), monitor, false);
            return;
        }

        if (naming.promptMode == EPromptMode::CREATE_SPECIAL && !naming.promptCustomSelected && useSelectedPreset(monitor))
            return;

        if (!naming.promptInput.text.empty()) {
            applySpecialNavigationResult(activePlugin()->navigator().createNamedSpecialWorkspace(naming.promptInput.text, monitor), monitor, true);
            return;
        }

        if (!useSelectedPreset(monitor))
            closePrompt(monitor);
    }

    void CNamingController::openNamedSpecialPrompt(const PHLMONITOR& monitor) {
        const auto names = activePlugin()->config().specialWorkspaceNames();

        auto&      naming = m_state;
        activePlugin()->textInputRepeater().stop();
        naming.promptMode           = EPromptMode::CREATE_SPECIAL;
        naming.promptCustomSelected = false;
        naming.promptInput.reset();
        naming.namedSpecialPromptIndex = names.empty() ? 0 : std::min(naming.namedSpecialPromptIndex, names.size() - 1);

        activePlugin()->layout().recalculateCards(monitor);
        damagePrompt(monitor);
    }

    void CNamingController::openRenameSpecialPrompt(const PHLMONITOR& monitor) {
        if (activePlugin()->selection().selectedRow() != ESelectedRow::SPECIAL)
            return;

        activePlugin()->layout().recalculateCards(monitor);

        const auto label = activePlugin()->selection().selectedSpecialWorkspaceLabel();
        if (label.empty())
            return;

        auto& naming = m_state;
        activePlugin()->textInputRepeater().stop();
        naming.promptMode           = EPromptMode::RENAME_SPECIAL;
        naming.promptCustomSelected = true;
        naming.promptInput.setText(label);

        damagePrompt(monitor);
    }

    void CNamingController::closePrompt(const PHLMONITOR& monitor) {
        if (!promptOpen())
            return;

        resetPromptState();
        damagePrompt(monitor);
    }

    void CNamingController::resetPromptState() {
        auto& naming                = m_state;
        naming.promptMode           = EPromptMode::NONE;
        naming.promptCustomSelected = false;
        naming.promptInput.reset();
        activePlugin()->textInputRepeater().stop();
    }

    void CNamingController::resetComponent() {
        resetPromptState();
    }

    void CNamingController::handleKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!promptOpen())
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            activePlugin()->textInputRepeater().stopFor(event.keycode);
            return;
        }

        const auto modifiers = activePlugin()->hyprland().keyboardModifiers();
        const bool ctrl      = modifiers.ctrl;
        const bool shift     = modifiers.shift;
        bool       dirty     = false;
        auto&      naming    = m_state;
        const auto command   = namingCommandForKey(event, modifiers, naming.promptMode, naming.promptInput.text.empty());
        const auto action    = textInputActionForKey(event, ctrl);

        switch (command) {
            case EShortcutCommand::CANCEL_TEXT: closePrompt(monitor); return;
            case EShortcutCommand::CONFIRM_TEXT: confirmPrompt(monitor); return;
            case EShortcutCommand::SELECT_NAMED_PRESET:
                if (movePresetSelection(presetSelectionDirection(event, modifiers), presetSelectionLoops(event, modifiers)))
                    damagePrompt(monitor);
                return;
            case EShortcutCommand::USE_NAMED_PRESET: useSelectedPreset(monitor); return;
            case EShortcutCommand::DELETE_TEXT_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_FORWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_BACKWARD:
            case EShortcutCommand::DELETE_TEXT_WORD_FORWARD:
            case EShortcutCommand::MOVE_TEXT_CURSOR:
            case EShortcutCommand::MOVE_TEXT_WORD:
            case EShortcutCommand::MOVE_TEXT_LINE_ENDS:
            case EShortcutCommand::CLEAR_TEXT:
            case EShortcutCommand::TYPE_TEXT:
                dirty = naming.promptInput.handleKey(event, ctrl, shift);

                if (activePlugin()->textInputRepeater().actionRepeats(action))
                    activePlugin()->textInputRepeater().start(action, event.keycode, namingRepeatTarget());
                break;
            default: return;
        }

        if (dirty)
            namingTextChanged(monitor);
    }

} // namespace hyprdeck
