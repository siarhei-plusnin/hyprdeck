#include "naming.hpp"

#include "config.hpp"
#include "keyboard.hpp"
#include "layout.hpp"
#include "navigation.hpp"
#include "selection.hpp"
#include "shortcut_catalog.hpp"
#include "state.hpp"
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
            if (monitor)
                g_pHyprRenderer->damageMonitor(monitor);
        }

        void syncCustomSelectionAfterEdit() {
            auto& naming = state().naming;
            if (naming.promptMode == EPromptMode::CREATE_SPECIAL)
                naming.promptCustomSelected = !naming.promptInput.text.empty();
        }

        bool movePresetSelection(const int direction) {
            auto& naming = state().naming;
            if (naming.promptMode != EPromptMode::CREATE_SPECIAL)
                return false;

            const auto names         = configuredSpecialWorkspaceNames();
            const bool hasCustomName = !naming.promptInput.text.empty();
            if (names.empty()) {
                if (hasCustomName && !naming.promptCustomSelected) {
                    naming.promptCustomSelected = true;
                    return true;
                }

                return false;
            }

            if (hasCustomName && naming.promptCustomSelected) {
                if (direction <= 0)
                    return false;

                naming.promptCustomSelected    = false;
                naming.namedSpecialPromptIndex = 0;
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

        int presetSelectionDirection(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            if (event.keycode == KEY_UP || (modifiers.ctrl && event.keycode == KEY_P))
                return -1;

            return 1;
        }

        EShortcutCommand namingCommandForKey(const IKeyboard::SKeyEvent event, const SKeyboardModifiers& modifiers) {
            auto& naming = state().naming;

            if (event.keycode == KEY_ESC)
                return EShortcutCommand::CANCEL_TEXT;

            if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
                return EShortcutCommand::CONFIRM_TEXT;

            if (naming.promptMode == EPromptMode::CREATE_SPECIAL && (event.keycode == KEY_UP || event.keycode == KEY_DOWN || (modifiers.ctrl && event.keycode == KEY_P) ||
                                                                        (modifiers.ctrl && event.keycode == KEY_N)))
                return EShortcutCommand::SELECT_NAMED_PRESET;

            if (event.keycode == KEY_SPACE && naming.promptMode == EPromptMode::CREATE_SPECIAL && naming.promptInput.text.empty())
                return EShortcutCommand::USE_NAMED_PRESET;

            const auto command = shortcutCommandForTextInputAction(textInputActionForKey(event, modifiers.ctrl));
            return command == EShortcutCommand::NONE ? EShortcutCommand::TYPE_TEXT : command;
        }

        bool useSelectedPreset(const PHLMONITOR& monitor) {
            const auto names = configuredSpecialWorkspaceNames();
            if (names.empty())
                return false;

            auto& naming                   = state().naming;
            naming.namedSpecialPromptIndex = std::min(naming.namedSpecialPromptIndex, names.size() - 1);
            if (!createNamedSpecialWorkspace(names[naming.namedSpecialPromptIndex], monitor))
                return false;

            resetNamingPromptState();
            return true;
        }

        STextInputState* namingPromptInput() {
            return &state().naming.promptInput;
        }

        bool namingRepeatActive() {
            return namingPromptOpen();
        }

        void namingTextChanged(const PHLMONITOR& monitor) {
            syncCustomSelectionAfterEdit();
            damagePrompt(monitor);
        }

        STextInputRepeatTarget namingRepeatTarget() {
            return STextInputRepeatTarget{.input = namingPromptInput, .active = namingRepeatActive, .changed = namingTextChanged};
        }

        void confirmNamingPrompt(const PHLMONITOR& monitor) {
            auto& naming = state().naming;
            if (naming.promptMode == EPromptMode::RENAME_SPECIAL) {
                if (renameSelectedSpecialWorkspace(naming.promptInput.text, monitor))
                    resetNamingPromptState();
                return;
            }

            if (naming.promptMode == EPromptMode::CREATE_SPECIAL && !naming.promptCustomSelected && useSelectedPreset(monitor))
                return;

            if (!naming.promptInput.text.empty()) {
                if (createNamedSpecialWorkspace(naming.promptInput.text, monitor))
                    resetNamingPromptState();
                return;
            }

            if (!useSelectedPreset(monitor))
                closeNamingPrompt(monitor);
        }

    } // namespace

    bool namingPromptOpen() {
        return state().naming.promptMode != EPromptMode::NONE;
    }

    void openNamedSpecialPrompt(const PHLMONITOR& monitor) {
        const auto names = configuredSpecialWorkspaceNames();

        auto&      naming = state().naming;
        stopTextInputRepeat();
        naming.promptMode           = EPromptMode::CREATE_SPECIAL;
        naming.promptCustomSelected = false;
        naming.promptInput.reset();
        naming.namedSpecialPromptIndex = names.empty() ? 0 : std::min(naming.namedSpecialPromptIndex, names.size() - 1);

        recalculateCards(monitor);
        damagePrompt(monitor);
    }

    void openRenameSpecialPrompt(const PHLMONITOR& monitor) {
        recalculateCards(monitor);

        const auto label = selectedSpecialWorkspaceLabel();
        if (label.empty())
            return;

        auto& naming = state().naming;
        stopTextInputRepeat();
        naming.promptMode           = EPromptMode::RENAME_SPECIAL;
        naming.promptCustomSelected = true;
        naming.promptInput.setText(label);

        damagePrompt(monitor);
    }

    void closeNamingPrompt(const PHLMONITOR& monitor) {
        if (!namingPromptOpen())
            return;

        resetNamingPromptState();
        damagePrompt(monitor);
    }

    void resetNamingPromptState() {
        auto& naming                = state().naming;
        naming.promptMode           = EPromptMode::NONE;
        naming.promptCustomSelected = false;
        naming.promptInput.reset();
        stopTextInputRepeat();
    }

    void resetNamingComponent() {
        resetNamingPromptState();
    }

    void handleNamingPromptKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!namingPromptOpen())
            return;

        if (event.state == WL_KEYBOARD_KEY_STATE_RELEASED) {
            stopTextInputRepeatFor(event.keycode);
            return;
        }

        const auto modifiers = keyboardModifiers();
        const bool ctrl      = modifiers.ctrl;
        const bool shift     = modifiers.shift;
        bool       dirty     = false;
        const auto command   = namingCommandForKey(event, modifiers);
        const auto action    = textInputActionForKey(event, ctrl);

        auto& naming = state().naming;
        switch (command) {
            case EShortcutCommand::CANCEL_TEXT: closeNamingPrompt(monitor); return;
            case EShortcutCommand::CONFIRM_TEXT: confirmNamingPrompt(monitor); return;
            case EShortcutCommand::SELECT_NAMED_PRESET: dirty = movePresetSelection(presetSelectionDirection(event, modifiers)); break;
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

                if (textInputActionRepeats(action))
                    startTextInputRepeat(action, event.keycode, namingRepeatTarget());
                break;
            default: return;
        }

        if (dirty)
            namingTextChanged(monitor);
    }

} // namespace hyprdeck
