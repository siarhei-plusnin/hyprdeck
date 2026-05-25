#include "workspaces.hpp"
#include "SharedDefs.hpp"
#include "macros.hpp"

#include <Compositor.hpp>
#include <algorithm>
#include <desktop/Workspace.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>

#include <string>

namespace hyprdeck {
    namespace {

        bool shouldCountWindow(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden())
                return false;

            return windowBelongsToMonitor(window, monitor) && isNormalNumericWorkspace(window->m_workspace);
        }

    } // namespace

    bool isNormalNumericWorkspace(const PHLWORKSPACE& workspace) {
        return workspace && !workspace->m_isSpecialWorkspace && workspace->m_id > 0;
    }

    bool windowBelongsToMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) {
        if (!window || !monitor)
            return false;

        const auto windowMonitor = window->m_monitor.lock();
        return windowMonitor && windowMonitor->m_id == monitor->m_id;
    }

    bool windowBelongsToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace) {
        return window && workspace && window->m_workspace == workspace;
    }

    bool workspaceHasAnyWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
        if (!workspace || !monitor)
            return false;

        for (const auto& window : g_pCompositor->m_windows) {
            if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden())
                continue;

            if (windowBelongsToMonitor(window, monitor) && windowBelongsToWorkspace(window, workspace))
                return true;
        }

        return false;
    }

    WORKSPACEID specialWorkspaceId(const PHLWORKSPACE& workspace) {
        if (!workspace)
            return WORKSPACE_INVALID;

        return workspace->m_id;
    }

    std::string specialWorkspaceLabel(const PHLWORKSPACE& workspace) {
        if (!workspace)
            return "special";

        auto label = workspace->m_name;
        if (label.starts_with("special:"))
            label = label.substr(8);

        return label.empty() ? "special" : label;
    }

    std::vector<PHLWORKSPACE> specialWorkspacesToShow(const PHLMONITOR& monitor) {
        std::vector<PHLWORKSPACE> workspaces;
        if (!monitor)
            return workspaces;

        for (const auto& workspace : g_pCompositor->getWorkspacesCopy()) {
            if (!workspace || !workspace->m_isSpecialWorkspace)
                continue;

            const auto workspaceMonitor = workspace->m_monitor.lock();
            if (workspaceMonitor && workspaceMonitor->m_id != monitor->m_id)
                continue;

            if (!workspaceHasAnyWindows(workspace, monitor) && workspace != monitor->m_activeSpecialWorkspace)
                continue;

            workspaces.push_back(workspace);
        }

        std::ranges::sort(workspaces, [](const auto& lhs, const auto& rhs) { return specialWorkspaceId(lhs) < specialWorkspaceId(rhs); });
        return workspaces;
    }

    WORKSPACEID lastWorkspaceToShow(const PHLMONITOR& monitor) {
        WORKSPACEID highestOccupied = 0;

        for (const auto& window : g_pCompositor->m_windows) {
            if (!shouldCountWindow(window, monitor))
                continue;

            highestOccupied = std::max(highestOccupied, window->m_workspace->m_id);
        }

        return std::max<WORKSPACEID>({static_cast<long>(3), highestOccupied + 1, activeNormalWorkspaceID(monitor) + 1});
    }

    WORKSPACEID activeNormalWorkspaceID(const PHLMONITOR& monitor) {
        if (monitor && isNormalNumericWorkspace(monitor->m_activeWorkspace))
            return monitor->m_activeWorkspace->m_id;

        return 1;
    }

    WORKSPACEID activeSpecialWorkspaceID(const PHLMONITOR& monitor) {
        if (monitor && monitor->m_activeSpecialWorkspace)
            return monitor->m_activeSpecialWorkspace->m_id;

        return WORKSPACE_INVALID;
    }

    int cardIndexByID(const std::vector<SWorkspaceCard>& cards, const WORKSPACEID id) {
        for (size_t i = 0; i < cards.size(); ++i) {
            if (cards[i].id == id)
                return static_cast<int>(i);
        }

        return -1;
    }

    bool cardIsActive(const SWorkspaceCard& card, const PHLMONITOR& monitor) {
        if (card.action != EWorkspaceCardAction::SWITCH)
            return false;

        if (card.special)
            return card.workspace && monitor && monitor->m_activeSpecialWorkspace == card.workspace;

        return card.id == activeNormalWorkspaceID(monitor);
    }

    bool cardIsSelected(const SWorkspaceCard& card) {
        const auto& selection = state().selection;
        if (card.special)
            return selection.selectedRow == ESelectedRow::SPECIAL && card.id == selection.selectedSpecialID;

        return selection.selectedRow == ESelectedRow::NORMAL && card.id == selection.selectedNormalID;
    }

    void ensureSelection(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        auto& current   = state();
        auto& layout    = current.layout;
        auto& selection = current.selection;

        if (layout.cards.empty()) {
            selection.selectedNormalID = WORKSPACE_INVALID;
            if (selection.selectedRow == ESelectedRow::NORMAL && !layout.specialCards.empty())
                selection.selectedRow = ESelectedRow::SPECIAL;
        } else if (cardIndexByID(layout.cards, selection.selectedNormalID) < 0) {
            const auto activeID = activeNormalWorkspaceID(monitor);
            selection.selectedNormalID = cardIndexByID(layout.cards, activeID) >= 0 ? activeID : layout.cards.front().id;
        }

        if (!layout.specialCards.empty()) {
            if (cardIndexByID(layout.specialCards, selection.selectedSpecialID) >= 0)
                return;

            const auto activeSpecial = monitor->m_activeSpecialWorkspace;
            if (activeSpecial && cardIndexByID(layout.specialCards, activeSpecial->m_id) >= 0)
                selection.selectedSpecialID = activeSpecial->m_id;
            else
                selection.selectedSpecialID = layout.specialCards.front().id;

            return;
        }

        selection.selectedSpecialID = WORKSPACE_INVALID;
        if (selection.selectedRow == ESelectedRow::SPECIAL)
            selection.selectedRow = ESelectedRow::NORMAL;
    }

} // namespace hyprdeck
