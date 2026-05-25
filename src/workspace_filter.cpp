#include "workspace_filter.hpp"

#include "colors.hpp"
#include "constants.hpp"
#include "keyboard.hpp"
#include "layout.hpp"
#include "overlays.hpp"
#include "shortcut_catalog.hpp"
#include "state.hpp"
#include "textinput.hpp"
#include "textinput_repeat.hpp"
#include "ui.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>
#include <render/Renderer.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <cctype>
#include <linux/input-event-codes.h>
#include <string>
#include <string_view>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        std::string lower(std::string_view value) {
            std::string lowered;
            lowered.reserve(value.size());
            for (const char character : value)
                lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));

            return lowered;
        }

        bool containsLowered(std::string_view value, std::string_view loweredQuery) {
            return lower(value).contains(loweredQuery);
        }

        bool windowReadyForFilter(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden() || window->m_pinned || windowIsExternalOverlay(window))
                return false;

            return windowBelongsToMonitor(window, monitor);
        }

        bool windowTextMatchesFilter(const PHLWINDOW& window, std::string_view loweredQuery) {
            return containsLowered(window->m_class, loweredQuery) || containsLowered(window->m_initialClass, loweredQuery) || containsLowered(window->m_title, loweredQuery) ||
                containsLowered(window->m_initialTitle, loweredQuery);
        }

        void damageFilter(const PHLMONITOR& monitor) {
            invalidateLayout();
            if (monitor)
                g_pHyprRenderer->damageMonitor(monitor);
        }

        void focusActiveWorkspace(const PHLMONITOR& monitor) {
            if (!monitor)
                return;

            auto& current     = state();
            auto& interaction = current.interaction;
            auto& layout      = current.layout;
            auto& selection   = current.selection;

            const auto activeSpecialID = activeSpecialWorkspaceID(monitor);
            const auto activeNormalID  = activeNormalWorkspaceID(monitor);

            interaction.suppressNextActiveCenter = false;
            layout.resetCamera                   = true;
            selection.selectedNormalID           = activeNormalID;
            selection.selectedSpecialID          = activeSpecialID;
            selection.selectedRow                = activeSpecialID != WORKSPACE_INVALID ? ESelectedRow::SPECIAL : ESelectedRow::NORMAL;

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

        double filterY(const PHLMONITOR& monitor, const double boxH) {
            const auto& layout   = state().layout;
            const auto  viewSize = monitor->m_transformedSize;
            double      y        = (viewSize.y - boxH) / 2.0;

            if (!layout.cards.empty() && !layout.specialCards.empty()) {
                const double normalBottom = layout.cards.front().box.y + layout.cards.front().box.h;
                const double specialTop   = layout.specialCards.front().box.y;
                y                         = normalBottom + ((specialTop - normalBottom - boxH) / 2.0);
            } else if (!layout.cards.empty()) {
                y = layout.cards.front().box.y + layout.cards.front().box.h + (MIN_ROW_GAP / 2.0);
            } else if (!layout.specialCards.empty()) {
                y = layout.specialCards.front().box.y - boxH - (MIN_ROW_GAP / 2.0);
            }

            return std::clamp(y, 24.0, std::max(24.0, viewSize.y - boxH - 24.0));
        }

        SP<Render::ITexture> filterTexture(const std::string& label, const int fontSize, const int weight, const CHyprColor& color = colors::textPrimary()) {
            return textTexture("workspace-filter", label, color, fontSize, weight, ETextCacheMode::NONE);
        }

        void renderFilterBox(const CBox& box) {
            addRect(expanded(box, 2.0), colors::componentBorder());
            addRect(box, colors::componentBackground());
        }

        CBox filterBox(const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const double boxW     = std::min(460.0, viewSize.x - 48.0);
            const double boxH     = 40.0;
            return CBox{(viewSize.x - boxW) / 2.0, filterY(monitor, boxH), boxW, boxH};
        }

        void renderFilterPromptBox(const PHLMONITOR& monitor, const std::string& label) {
            const auto texture = filterTexture(label, 20, 700);
            if (!texture || !texture->ok())
                return;

            const auto box = filterBox(monitor);

            renderFilterBox(box);
            addTexture(texture, CBox{box.x + 14.0, box.y + ((box.h - texture->m_size.y) / 2.0), texture->m_size.x, texture->m_size.y}, 1.0F, 0, box);
        }

        void renderAppliedFilterBox(const PHLMONITOR& monitor, const std::string& text) {
            const auto label = filterTexture("Filter: " + text, 20, 700);
            if (!label || !label->ok())
                return;

            const auto   box     = filterBox(monitor);
            const double padding = 14.0;

            renderFilterBox(box);
            const CBox labelClip{box.x + padding, box.y, box.w - (padding * 2.0), box.h};
            addTexture(label, CBox{labelClip.x, box.y + ((box.h - label->m_size.y) / 2.0), label->m_size.x, label->m_size.y}, 1.0F, 0, labelClip);
        }

    } // namespace

    bool workspaceFilterPromptOpen() {
        return state().filter.promptOpen;
    }

    std::string_view workspaceFilterText() {
        const auto& filter = state().filter;
        return filter.promptOpen ? std::string_view{filter.promptInput.text} : std::string_view{filter.text};
    }

    bool workspaceFilterActive() {
        return !workspaceFilterText().empty();
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

        if (monitor) {
            focusActiveWorkspace(monitor);
            g_pHyprRenderer->damageMonitor(monitor);
        } else
            invalidateLayout();
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

    void renderWorkspaceFilter(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        const auto& filter = state().filter;
        if (filter.promptOpen) {
            const auto label = filter.promptInput.text.empty() ? std::string{"Filter workspaces"} : "Filter: " + filter.promptInput.withCursor();
            renderFilterPromptBox(monitor, label);
            return;
        }

        if (filter.text.empty())
            return;

        renderAppliedFilterBox(monitor, filter.text);
    }

    bool workspaceMatchesFilter(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
        if (!workspace || !monitor)
            return false;

        const auto loweredQuery = lower(workspaceFilterText());
        if (loweredQuery.empty())
            return true;

        for (const auto& window : g_pCompositor->m_windows) {
            if (!windowReadyForFilter(window, monitor) || !windowBelongsToWorkspace(window, workspace))
                continue;

            if (windowTextMatchesFilter(window, loweredQuery))
                return true;
        }

        return false;
    }

    std::vector<WORKSPACEID> filteredNormalWorkspaceIDs(const PHLMONITOR& monitor) {
        std::vector<WORKSPACEID> ids;
        if (!monitor)
            return ids;

        const auto loweredQuery = lower(workspaceFilterText());
        if (loweredQuery.empty())
            return ids;

        for (const auto& window : g_pCompositor->m_windows) {
            if (!windowReadyForFilter(window, monitor) || !isNormalNumericWorkspace(window->m_workspace))
                continue;

            if (windowTextMatchesFilter(window, loweredQuery))
                ids.push_back(window->m_workspace->m_id);
        }

        std::ranges::sort(ids);
        const auto duplicates = std::ranges::unique(ids);
        ids.erase(duplicates.begin(), duplicates.end());
        return ids;
    }

} // namespace hyprdeck
