#include "selection.hpp"

#include "confirmation.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "navigation.hpp"
#include "overview.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <render/Renderer.hpp>

#include <algorithm>

namespace hyprdeck {

    const SWorkspaceCard* selectedSpecialCard() {
        const auto& current   = state();
        const auto& layout    = current.layout;
        const auto& selection = current.selection;
        const int   index     = cardIndexByID(layout.specialCards, selection.selectedSpecialID);
        if (index < 0)
            return nullptr;

        return &layout.specialCards[index];
    }

    const SWorkspaceCard* selectedNormalCard() {
        const auto& current   = state();
        const auto& layout    = current.layout;
        const auto& selection = current.selection;
        const int   index     = cardIndexByID(layout.cards, selection.selectedNormalID);
        if (index < 0)
            return nullptr;

        return &layout.cards[index];
    }

    std::string selectedSpecialWorkspaceLabel() {
        const auto* card = selectedSpecialCard();
        if (!card || !card->workspace)
            return "";

        return specialWorkspaceLabel(card->workspace);
    }

    void selectWorkspaceAt(const Vector2D& position, const PHLMONITOR& monitor) {
        const auto card = cardAt(position);
        if (!card) {
            closeOverview();
            return;
        }

        if (card->action == EWorkspaceCardAction::SWITCH && !card->special && (cardIsActive(*card, monitor) || cardIsSelected(*card))) {
            monitor->setSpecialWorkspace(nullptr);
            closeOverview();
            return;
        }

        switchWorkspaceCard(*card, monitor, false, true);
    }

    void selectKeyboardRow(const ESelectedRow row, const PHLMONITOR& monitor) {
        auto& current = state();
        auto& layout = current.layout;
        auto& selection = current.selection;
        resetNamingPromptState();

        if (row == ESelectedRow::NORMAL)
            cleanupPendingSpecialWorkspace(monitor);

        recalculateCards(monitor);

        if (row == ESelectedRow::SPECIAL && layout.specialCards.empty()) {
            createSimpleSpecialWorkspace(monitor);
            return;
        }

        selection.selectedRow = row;
        invalidateLayout();
        ensureSelection(monitor);

        if (selection.selectedRow == ESelectedRow::SPECIAL)
            centerSpecialCard(cardIndexByID(layout.specialCards, selection.selectedSpecialID));
        else
            centerNormalCard(cardIndexByID(layout.cards, selection.selectedNormalID));

        g_pHyprRenderer->damageMonitor(monitor);
    }

    namespace {

        void confirmKeyboardSelection(const PHLMONITOR& monitor) {
            recalculateCards(monitor);
            ensureSelection(monitor);

            auto& current   = state();
            auto& layout    = current.layout;
            auto& selection = current.selection;
            if (selection.selectedRow == ESelectedRow::NORMAL) {
                const int index = cardIndexByID(layout.cards, selection.selectedNormalID);
                if (index < 0)
                    return;

                if (monitor->m_activeSpecialWorkspace)
                    monitor->setSpecialWorkspace(nullptr);

                closeOverview();

                return;
            }

            if (layout.specialCards.empty())
                return;

            const int index = cardIndexByID(layout.specialCards, selection.selectedSpecialID);
            if (index < 0)
                return;

            centerSpecialCard(index);
            switchWorkspaceCard(layout.specialCards[index], monitor, false);
        }

    } // namespace

    void spaceKeyboardSelection(const PHLMONITOR& monitor) {
        recalculateCards(monitor);
        ensureSelection(monitor);

        auto& selection = state().selection;
        if (selection.selectedRow == ESelectedRow::NORMAL) {
            if (monitor->m_activeSpecialWorkspace)
                monitor->setSpecialWorkspace(nullptr);

            if (const auto* card = selectedNormalCard())
                switchWorkspaceCard(*card, monitor, false);
            else {
                invalidateLayout();
                g_pHyprRenderer->damageMonitor(monitor);
            }
            return;
        }

        confirmKeyboardSelection(monitor);
    }

    void openSelectedSpecialAndClose(const PHLMONITOR& monitor) {
        recalculateCards(monitor);
        ensureSelection(monitor);

        auto& current   = state();
        auto& layout    = current.layout;
        auto& selection = current.selection;
        if (selection.selectedRow == ESelectedRow::NORMAL) {
            if (monitor->m_activeSpecialWorkspace)
                monitor->setSpecialWorkspace(nullptr);

            if (const auto* card = selectedNormalCard())
                switchWorkspaceCard(*card, monitor, true);
            else
                closeOverview();
            return;
        }

        if (layout.specialCards.empty())
            return;

        const int index = cardIndexByID(layout.specialCards, selection.selectedSpecialID);
        if (index < 0)
            return;

        const auto& card = layout.specialCards[index];
        if (card.workspace && monitor->m_activeSpecialWorkspace != card.workspace)
            monitor->setSpecialWorkspace(card.workspace);

        closeOverview();
    }

    void switchNormalWorkspaceByID(const WORKSPACEID id, const PHLMONITOR& monitor) {
        if (!monitor || id < 1)
            return;

        cleanupPendingSpecialWorkspace(monitor);

        const auto workspace = g_pCompositor->getWorkspaceByID(id);
        switchWorkspaceCard(
            SWorkspaceCard{
                .id        = id,
                .workspace = isNormalNumericWorkspace(workspace) ? workspace : nullptr,
                .label     = std::to_string(id),
                .special   = false,
            },
            monitor, false);

        recalculateCards(monitor);
        centerNormalCard(cardIndexByID(state().layout.cards, id));
        g_pHyprRenderer->damageMonitor(monitor);
    }

    void closeSelectedWorkspaceWindows(const PHLMONITOR& monitor) {
        recalculateCards(monitor);

        auto& current   = state();
        auto& layout    = current.layout;
        auto& selection = current.selection;
        if (selection.selectedRow == ESelectedRow::NORMAL) {
            const auto* card = selectedNormalCard();
            if (!card || card->action != EWorkspaceCardAction::SWITCH || !isNormalNumericWorkspace(card->workspace))
                return;

            openCloseNormalWorkspaceConfirmation(card->workspace->m_id, monitor);
            return;
        }

        if (selection.selectedRow != ESelectedRow::SPECIAL)
            return;

        const auto* card = selectedSpecialCard();
        if (!card || card->action != EWorkspaceCardAction::SWITCH || !card->workspace)
            return;

        const int oldIndex = cardIndexByID(layout.specialCards, selection.selectedSpecialID);

        closeWorkspaceWindows(card->workspace);

        if (monitor->m_activeSpecialWorkspace == card->workspace)
            monitor->setSpecialWorkspace(nullptr);

        selection.selectedRow       = ESelectedRow::SPECIAL;
        selection.selectedSpecialID = WORKSPACE_INVALID;
        if (selection.pendingSpecialID == card->id)
            selection.pendingSpecialID = WORKSPACE_INVALID;

        invalidateLayout();

        recalculateCards(monitor);

        if (layout.specialCards.empty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
        } else {
            const int nextIndex = oldIndex <= 0 ? 0 : oldIndex - 1;

            selection.selectedSpecialID = layout.specialCards[nextIndex].id;
            centerSpecialCard(nextIndex);
        }

        g_pHyprRenderer->damageMonitor(monitor);
    }

    void moveKeyboardSelection(const int direction, const PHLMONITOR& monitor) {
        recalculateCards(monitor);

        auto& current = state();
        auto& layout = current.layout;
        auto& selection = current.selection;
        if (selection.selectedRow == ESelectedRow::SPECIAL && layout.specialCards.empty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
            invalidateLayout();
            return;
        }

        const bool special = selection.selectedRow == ESelectedRow::SPECIAL;
        auto&      cards   = special ? layout.specialCards : layout.cards;

        if (cards.empty())
            return;

        int index = cardIndexByID(cards, special ? selection.selectedSpecialID : selection.selectedNormalID);
        if (!special) {
            const int activeIndex = cardIndexByID(cards, activeNormalWorkspaceID(monitor));
            if (activeIndex >= 0 && index != activeIndex)
                index = activeIndex;
        }

        if (index < 0)
            index = direction < 0 ? static_cast<int>(cards.size()) - 1 : 0;

        if (special) {
            const int nextIndex = std::clamp(index + direction, 0, static_cast<int>(cards.size()) - 1);
            if (nextIndex == index)
                return;

            selection.selectedSpecialID = cards[nextIndex].id;
            invalidateLayout();
            centerSpecialCard(nextIndex);
            g_pHyprRenderer->damageMonitor(monitor);
            return;
        }

        const auto* target = &cards[index];
        if (cardIsActive(*target, monitor)) {
            const int nextIndex = std::clamp(index + direction, 0, static_cast<int>(cards.size()) - 1);
            if (nextIndex == index)
                return;

            index  = nextIndex;
            target = &cards[index];
        }

        selection.selectedNormalID = target->id;
        invalidateLayout();
        centerNormalCard(index);

        switchWorkspaceCard(*target, monitor, false);
    }

    void jumpKeyboardSelection(const int direction, const PHLMONITOR& monitor) {
        recalculateCards(monitor);

        auto& current = state();
        auto& layout = current.layout;
        auto& selection = current.selection;
        if (selection.selectedRow == ESelectedRow::SPECIAL && layout.specialCards.empty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
            invalidateLayout();
            return;
        }

        const bool special = selection.selectedRow == ESelectedRow::SPECIAL;
        auto&      cards   = special ? layout.specialCards : layout.cards;

        if (cards.empty())
            return;

        const int index = direction < 0 ? 0 : static_cast<int>(cards.size()) - 1;

        if (special) {
            selection.selectedSpecialID = cards[index].id;
            invalidateLayout();
            centerSpecialCard(index);
            g_pHyprRenderer->damageMonitor(monitor);
            return;
        }

        const auto& target = cards[index];
        selection.selectedNormalID = target.id;
        invalidateLayout();
        centerNormalCard(index);

        if (cardIsActive(target, monitor))
            g_pHyprRenderer->damageMonitor(monitor);
        else
            switchWorkspaceCard(target, monitor, false);
    }

} // namespace hyprdeck
