#include "selection.hpp"

#include "confirmation.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "navigation.hpp"
#include "overview.hpp"
#include "plugin.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <desktop/Workspace.hpp>
#include <render/Renderer.hpp>

#include <algorithm>

namespace hyprdeck {
    SSelectionState CSelectionController::snapshot() const {
        return m_state;
    }

    ESelectedRow CSelectionController::selectedRow() const {
        return m_state.selectedRow;
    }

    WORKSPACEID CSelectionController::selectedNormalID() const {
        return m_state.selectedNormalID;
    }

    WORKSPACEID CSelectionController::selectedSpecialID() const {
        return m_state.selectedSpecialID;
    }

    WORKSPACEID CSelectionController::lastActiveNormalID() const {
        return m_state.lastActiveNormalID;
    }

    WORKSPACEID CSelectionController::lastActiveSpecialID() const {
        return m_state.lastActiveSpecialID;
    }

    void CSelectionController::resetState() {
        m_state.selectedRow         = ESelectedRow::NORMAL;
        m_state.selectedNormalID    = 1;
        m_state.selectedSpecialID   = WORKSPACE_INVALID;
        m_state.lastActiveNormalID  = WORKSPACE_INVALID;
        m_state.lastActiveSpecialID = WORKSPACE_INVALID;
    }

    void CSelectionController::setSelectedRow(const ESelectedRow row) {
        m_state.selectedRow = row;
    }

    void CSelectionController::setSelectedNormalID(const WORKSPACEID id) {
        m_state.selectedNormalID = id;
    }

    void CSelectionController::setSelectedSpecialID(const WORKSPACEID id) {
        m_state.selectedSpecialID = id;
    }

    void CSelectionController::setLastActiveNormalID(const WORKSPACEID id) {
        m_state.lastActiveNormalID = id;
    }

    void CSelectionController::setLastActiveSpecialID(const WORKSPACEID id) {
        m_state.lastActiveSpecialID = id;
    }

    void CSelectionController::setActiveSelection(const WORKSPACEID activeNormalID, const WORKSPACEID activeSpecialID) {
        m_state.selectedNormalID    = activeNormalID;
        m_state.selectedSpecialID   = activeSpecialID;
        m_state.selectedRow         = activeSpecialID != WORKSPACE_INVALID ? ESelectedRow::SPECIAL : ESelectedRow::NORMAL;
        m_state.lastActiveNormalID  = activeNormalID;
        m_state.lastActiveSpecialID = activeSpecialID;
    }

    void CSelectionController::ensureSelection(const PHLMONITOR& monitor) {
        const auto& cards        = activePlugin()->layout().cards();
        const auto& specialCards = activePlugin()->layout().specialCards();
        auto&       selection    = m_state;

        if (cards.empty()) {
            selection.selectedNormalID = WORKSPACE_INVALID;
            if (selection.selectedRow == ESelectedRow::NORMAL && !specialCards.empty())
                selection.selectedRow = ESelectedRow::SPECIAL;
        } else if (activePlugin()->workspaces().cardIndexByID(cards, selection.selectedNormalID) < 0) {
            const auto activeID        = activePlugin()->workspaces().activeNormalWorkspaceID(monitor);
            selection.selectedNormalID = activePlugin()->workspaces().cardIndexByID(cards, activeID) >= 0 ? activeID : cards.front().id;
        }

        if (!specialCards.empty()) {
            if (activePlugin()->workspaces().cardIndexByID(specialCards, selection.selectedSpecialID) >= 0)
                return;

            const auto activeSpecial = monitor->m_activeSpecialWorkspace;
            if (activeSpecial && activePlugin()->workspaces().cardIndexByID(specialCards, activeSpecial->m_id) >= 0)
                selection.selectedSpecialID = activeSpecial->m_id;
            else
                selection.selectedSpecialID = specialCards.front().id;

            return;
        }

        selection.selectedSpecialID = WORKSPACE_INVALID;
        if (selection.selectedRow == ESelectedRow::SPECIAL)
            selection.selectedRow = ESelectedRow::NORMAL;
    }

    bool CSelectionController::applyNavigationResult(const SWorkspaceNavigationResult& result) {
        if (!result.success)
            return false;

        if (result.selectedRow)
            setSelectedRow(*result.selectedRow);
        if (result.selectedNormalID)
            setSelectedNormalID(*result.selectedNormalID);
        if (result.selectedSpecialID)
            setSelectedSpecialID(*result.selectedSpecialID);

        return true;
    }

    const SWorkspaceCard* CSelectionController::selectedSpecialCard() const {
        const auto& specialCards = activePlugin()->layout().specialCards();
        const auto& selection    = m_state;
        const int   index        = activePlugin()->workspaces().cardIndexByID(specialCards, selection.selectedSpecialID);
        if (index < 0)
            return nullptr;

        return &specialCards[index];
    }

    const SWorkspaceCard* CSelectionController::selectedNormalCard() const {
        const auto& cards     = activePlugin()->layout().cards();
        const auto& selection = m_state;
        const int   index     = activePlugin()->workspaces().cardIndexByID(cards, selection.selectedNormalID);
        if (index < 0)
            return nullptr;

        return &cards[index];
    }

    std::string CSelectionController::selectedSpecialWorkspaceLabel() const {
        const auto* card = selectedSpecialCard();
        if (!card || !card->workspace)
            return "";

        return activePlugin()->workspaces().specialWorkspaceLabel(card->workspace);
    }

    void CSelectionController::commitWorkspaceSelection(const PHLMONITOR& monitor) const {
        if (!monitor)
            return;

        auto&      hyprland          = activePlugin()->hyprland();
        auto&      overview          = activePlugin()->overview();
        const auto focusedMonitor    = hyprland.activeMonitor();
        const bool switchingOutput   = focusedMonitor && focusedMonitor->m_id != monitor->m_id;
        const auto workspace         = monitor->m_activeSpecialWorkspace ? monitor->m_activeSpecialWorkspace : monitor->m_activeWorkspace;
        const bool hasFocusCandidate = workspace && workspace->getFocusCandidate();

        if (!overview.prepareWorkspaceCommit())
            return;

        if (switchingOutput)
            hyprland.focusMonitor(monitor);

        const auto pointerMonitor = hyprland.monitorFromCursor();
        if (pointerMonitor && pointerMonitor->m_id == monitor->m_id && (!switchingOutput || !hasFocusCandidate))
            hyprland.refreshPointerFocus();

        overview.close();
    }

    void CSelectionController::selectWorkspaceAt(const Vector2D& position, const PHLMONITOR& monitor) {
        const auto card = activePlugin()->layout().cardAt(position);
        if (!card) {
            activePlugin()->overview().close();
            return;
        }

        const bool native = activePlugin()->workspaces().cardIsNative(*card, monitor);
        if (!card->special && native && (activePlugin()->workspaces().cardIsActive(*card, monitor) || activePlugin()->workspaces().cardIsSelected(*card))) {
            applyNavigationResult(activePlugin()->navigator().hideActiveSpecialWorkspace(monitor));
            if (!activePlugin()->workspaces().cardIsActive(*card, monitor))
                applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor));
            commitWorkspaceSelection(monitor);
            return;
        }

        if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor))) {
            activePlugin()->layout().invalidate();
            activePlugin()->overview().damageHost();
        }
    }

    void CSelectionController::selectRow(const ESelectedRow row, const PHLMONITOR& monitor) {
        auto& selection = m_state;
        activePlugin()->naming().resetPromptState();

        activePlugin()->layout().recalculateCards(monitor);

        if (row == ESelectedRow::SPECIAL && activePlugin()->layout().specialCardsEmpty()) {
            const auto result = activePlugin()->navigator().createSimpleSpecialWorkspace(monitor);
            if (applyNavigationResult(result)) {
                activePlugin()->naming().resetPromptState();
                activePlugin()->layout().invalidate();
                activePlugin()->layout().recalculateCards(monitor);
                if (result.selectedSpecialID) {
                    activePlugin()->layout().centerSpecialCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), *result.selectedSpecialID));
                    activePlugin()->animations().startSpecialCardAppearance(*result.selectedSpecialID, activePlugin()->overview().hostMonitor());
                }
                activePlugin()->overview().damageHost();
            }
            return;
        }

        if (row == ESelectedRow::NORMAL) {
            const auto& specialCards = activePlugin()->layout().specialCards();
            if (specialCards.size() == 1 && specialCards.front().workspace && monitor->m_activeSpecialWorkspace == specialCards.front().workspace &&
                !activePlugin()->workspaces().workspaceHasAnyWindows(specialCards.front().workspace, monitor)) {
                applyNavigationResult(activePlugin()->navigator().hideActiveSpecialWorkspace(monitor));
                activePlugin()->layout().invalidate();
                activePlugin()->layout().recalculateCards(monitor);
            }
        }

        selection.selectedRow = row;
        activePlugin()->layout().invalidate();
        ensureSelection(monitor);

        if (selection.selectedRow == ESelectedRow::SPECIAL)
            activePlugin()->layout().centerSpecialCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), selection.selectedSpecialID));
        else
            activePlugin()->layout().centerNormalCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().cards(), selection.selectedNormalID));

        activePlugin()->overview().damageHost();
    }

    void CSelectionController::toggleSelection(const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);
        ensureSelection(monitor);

        auto& selection = m_state;
        if (selection.selectedRow == ESelectedRow::NORMAL) {
            applyNavigationResult(activePlugin()->navigator().hideActiveSpecialWorkspace(monitor));

            if (const auto* card = selectedNormalCard()) {
                if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor))) {
                    activePlugin()->layout().invalidate();
                    activePlugin()->overview().damageHost();
                }
            } else {
                activePlugin()->layout().invalidate();
                activePlugin()->overview().damageHost();
            }
            return;
        }

        if (const auto* card = selectedSpecialCard()) {
            if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor))) {
                activePlugin()->layout().invalidate();
                activePlugin()->overview().damageHost();
            }
        } else {
            activePlugin()->layout().invalidate();
            activePlugin()->overview().damageHost();
        }
    }

    void CSelectionController::openSelection(const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);
        ensureSelection(monitor);

        auto& selection = m_state;
        if (selection.selectedRow == ESelectedRow::NORMAL) {
            applyNavigationResult(activePlugin()->navigator().hideActiveSpecialWorkspace(monitor));

            if (const auto* card = selectedNormalCard()) {
                if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor)))
                    commitWorkspaceSelection(monitor);
            } else
                activePlugin()->overview().close();
            return;
        }

        if (activePlugin()->layout().specialCardsEmpty())
            return;

        if (const auto* card = selectedSpecialCard()) {
            if (card->workspace && monitor->m_activeSpecialWorkspace != card->workspace)
                applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*card, monitor));

            commitWorkspaceSelection(monitor);
            return;
        }

        activePlugin()->overview().close();
    }

    void CSelectionController::switchNormalWorkspaceByID(const WORKSPACEID id, const PHLMONITOR& monitor) {
        if (id < 1)
            return;

        activePlugin()->layout().recalculateCards(monitor);
        const auto& cards = activePlugin()->layout().cards();
        const int   index = activePlugin()->workspaces().cardIndexByID(cards, id);
        if (index < 0)
            return;

        const auto& card = cards[index];
        if (!activePlugin()->workspaces().cardIsNative(card, monitor) && m_state.selectedNormalID != id) {
            m_state.selectedRow      = ESelectedRow::NORMAL;
            m_state.selectedNormalID = id;
            activePlugin()->layout().invalidate();
            activePlugin()->layout().centerNormalCard(index);
            activePlugin()->overview().damageHost();
            return;
        }

        if (id == activePlugin()->workspaces().activeNormalWorkspaceID(monitor)) {
            applyNavigationResult(activePlugin()->navigator().hideActiveSpecialWorkspace(monitor));
            commitWorkspaceSelection(monitor);
            return;
        }

        applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(card, monitor));

        activePlugin()->layout().invalidate();
        activePlugin()->layout().recalculateCards(monitor);
        activePlugin()->layout().centerNormalCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().cards(), id));
        activePlugin()->overview().damageHost();
    }

    void CSelectionController::closeSelectedNormalWorkspaceWindows(const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);

        const auto* card = selectedNormalCard();
        if (!card || !activePlugin()->workspaces().isNormalWorkspace(card->workspace))
            return;

        const auto owner = activePlugin()->workspaces().workspaceMonitor(card->workspace);
        if (!owner || !activePlugin()->workspaces().workspaceHasAnyWindows(card->workspace, owner))
            return;

        activePlugin()->confirmation().openCloseNormalWorkspaceConfirmation(card->workspace->m_id, owner);
    }

    void CSelectionController::closeSelectedSpecialWorkspaceWindows(const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);

        auto&       selection = m_state;
        const auto* card      = selectedSpecialCard();
        if (!card || !card->workspace)
            return;

        const auto closingCard = *card;
        const int  oldIndex    = activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), selection.selectedSpecialID);

        const auto owner = activePlugin()->workspaces().workspaceMonitor(card->workspace);
        activePlugin()->animations().startSpecialCardClose(closingCard, activePlugin()->overview().hostMonitor());
        activePlugin()->navigator().closeWorkspaceWindows(card->workspace, owner);

        if (owner && owner->m_activeSpecialWorkspace == card->workspace)
            activePlugin()->navigator().hideActiveSpecialWorkspace(owner, false);

        selection.selectedRow       = ESelectedRow::SPECIAL;
        selection.selectedSpecialID = WORKSPACE_INVALID;

        activePlugin()->layout().invalidate();

        activePlugin()->layout().recalculateCards(monitor);

        if (activePlugin()->layout().specialCardsEmpty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
        } else {
            const int nextIndex = oldIndex <= 0 ? 0 : oldIndex - 1;

            selection.selectedSpecialID = activePlugin()->layout().specialCards()[nextIndex].id;
            activePlugin()->layout().centerSpecialCard(nextIndex);
        }

        activePlugin()->overview().damageHost();
    }

    void CSelectionController::moveSelection(const int direction, const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);

        auto& selection = m_state;
        if (selection.selectedRow == ESelectedRow::SPECIAL && activePlugin()->layout().specialCardsEmpty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
            activePlugin()->layout().invalidate();
            return;
        }

        const bool  special = selection.selectedRow == ESelectedRow::SPECIAL;
        const auto& cards   = special ? activePlugin()->layout().specialCards() : activePlugin()->layout().cards();

        if (cards.empty())
            return;

        int index = activePlugin()->workspaces().cardIndexByID(cards, special ? selection.selectedSpecialID : selection.selectedNormalID);
        if (!special && index < 0)
            index = activePlugin()->workspaces().cardIndexByID(cards, activePlugin()->workspaces().activeNormalWorkspaceID(monitor));

        if (index < 0)
            index = direction < 0 ? static_cast<int>(cards.size()) - 1 : 0;

        if (special) {
            const int nextIndex = std::clamp(index + direction, 0, static_cast<int>(cards.size()) - 1);
            if (nextIndex == index)
                return;

            selection.selectedSpecialID = cards[nextIndex].id;
            activePlugin()->layout().invalidate();
            activePlugin()->layout().centerSpecialCard(nextIndex);
            activePlugin()->overview().damageHost();
            return;
        }

        const int nextIndex = std::clamp(index + direction, 0, static_cast<int>(cards.size()) - 1);
        if (nextIndex == index)
            return;

        index              = nextIndex;
        const auto* target = &cards[index];

        selection.selectedNormalID = target->id;
        activePlugin()->layout().invalidate();
        activePlugin()->layout().centerNormalCard(index);

        if (!activePlugin()->workspaces().cardIsNative(*target, monitor)) {
            activePlugin()->overview().damageHost();
            return;
        }

        if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(*target, monitor)))
            activePlugin()->overview().damageHost();
    }

    void CSelectionController::moveSelectedSpecialWorkspace(const int direction, const bool toEdge, const PHLMONITOR& monitor) {
        if (m_state.selectedRow != ESelectedRow::SPECIAL || activePlugin()->workspaceFilter().applied())
            return;

        activePlugin()->layout().recalculateCards(monitor);
        if (!activePlugin()->workspaces().moveSpecialWorkspaceInOrder(m_state.selectedSpecialID, direction))
            return;
        while (toEdge && activePlugin()->workspaces().moveSpecialWorkspaceInOrder(m_state.selectedSpecialID, direction)) {}

        activePlugin()->animations().cancelSpecialCameraAnimation();
        activePlugin()->layout().invalidate();
        activePlugin()->layout().recalculateCards(monitor);
        activePlugin()->layout().centerSpecialCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), m_state.selectedSpecialID));
        activePlugin()->overview().damageHost();
    }

    void CSelectionController::jumpSelection(const int direction, const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);

        auto& selection = m_state;
        if (selection.selectedRow == ESelectedRow::SPECIAL && activePlugin()->layout().specialCardsEmpty()) {
            selection.selectedRow = ESelectedRow::NORMAL;
            activePlugin()->layout().invalidate();
            return;
        }

        const bool  special = selection.selectedRow == ESelectedRow::SPECIAL;
        const auto& cards   = special ? activePlugin()->layout().specialCards() : activePlugin()->layout().cards();

        if (cards.empty())
            return;

        const int index = direction < 0 ? 0 : static_cast<int>(cards.size()) - 1;

        if (special) {
            selection.selectedSpecialID = cards[index].id;
            activePlugin()->layout().invalidate();
            activePlugin()->layout().centerSpecialCard(index);
            activePlugin()->overview().damageHost();
            return;
        }

        const auto& target         = cards[index];
        selection.selectedNormalID = target.id;
        activePlugin()->layout().invalidate();
        activePlugin()->layout().centerNormalCard(index);

        if (!activePlugin()->workspaces().cardIsNative(target, monitor)) {
            activePlugin()->overview().damageHost();
            return;
        }

        if (activePlugin()->workspaces().cardIsActive(target, monitor))
            activePlugin()->overview().damageHost();
        else if (applyNavigationResult(activePlugin()->navigator().switchWorkspaceCard(target, monitor)))
            activePlugin()->overview().damageHost();
    }

} // namespace hyprdeck
