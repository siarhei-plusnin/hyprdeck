#include "workspaces.hpp"
#include "SharedDefs.hpp"
#include "macros.hpp"
#include "plugin.hpp"
#include "strings.hpp"

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

            return activePlugin()->workspaces().windowBelongsToMonitor(window, monitor) && activePlugin()->workspaces().isNormalWorkspace(window->m_workspace);
        }

    } // namespace

    bool CWorkspaceRepository::isNormalWorkspace(const PHLWORKSPACE& workspace) const {
        return workspace && !workspace->m_isSpecialWorkspace && workspace->m_id > 0;
    }

    bool CWorkspaceRepository::windowBelongsToMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) const {
        if (!window)
            return false;

        const auto windowMonitor = window->m_monitor.lock();
        return windowMonitor && windowMonitor->m_id == monitor->m_id;
    }

    bool CWorkspaceRepository::windowBelongsToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace) const {
        return window && workspace && window->m_workspace == workspace;
    }

    bool CWorkspaceRepository::workspaceHasAnyWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const {
        if (!workspace)
            return false;

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden())
                continue;

            if (windowBelongsToMonitor(window, monitor) && windowBelongsToWorkspace(window, workspace))
                return true;
        }

        return false;
    }

    WORKSPACEID CWorkspaceRepository::specialWorkspaceId(const PHLWORKSPACE& workspace) const {
        if (!workspace)
            return WORKSPACE_INVALID;

        return workspace->m_id;
    }

    std::string CWorkspaceRepository::specialWorkspaceLabel(const PHLWORKSPACE& workspace) const {
        if (!workspace)
            return "special";

        auto label = strings::stripPrefix(workspace->m_name, "special:");

        return label.empty() ? "special" : label;
    }

    std::vector<PHLWORKSPACE> CWorkspaceRepository::specialWorkspacesToShow(const PHLMONITOR& monitor) const {
        std::vector<PHLWORKSPACE> workspaces;
        for (const auto& workspace : activePlugin()->hyprland().workspacesCopy()) {
            if (!workspace || !workspace->m_isSpecialWorkspace)
                continue;

            const auto workspaceMonitor = workspace->m_monitor.lock();
            if (workspaceMonitor && workspaceMonitor->m_id != monitor->m_id)
                continue;

            if (!workspaceHasAnyWindows(workspace, monitor) && workspace != monitor->m_activeSpecialWorkspace)
                continue;

            workspaces.push_back(workspace);
        }

        std::ranges::sort(workspaces, [this](const auto& lhs, const auto& rhs) { return specialWorkspaceId(lhs) < specialWorkspaceId(rhs); });
        return workspaces;
    }

    WORKSPACEID CWorkspaceRepository::lastWorkspaceToShow(const PHLMONITOR& monitor) const {
        WORKSPACEID highestOccupied = 0;

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (!shouldCountWindow(window, monitor))
                continue;

            highestOccupied = std::max(highestOccupied, window->m_workspace->m_id);
        }

        return std::max<WORKSPACEID>({static_cast<long>(3), highestOccupied + 1, activeNormalWorkspaceID(monitor) + 1});
    }

    WORKSPACEID CWorkspaceRepository::activeNormalWorkspaceID(const PHLMONITOR& monitor) const {
        if (isNormalWorkspace(monitor->m_activeWorkspace))
            return monitor->m_activeWorkspace->m_id;

        return 1;
    }

    WORKSPACEID CWorkspaceRepository::activeSpecialWorkspaceID(const PHLMONITOR& monitor) const {
        if (monitor->m_activeSpecialWorkspace)
            return monitor->m_activeSpecialWorkspace->m_id;

        return WORKSPACE_INVALID;
    }

    int CWorkspaceRepository::cardIndexByID(const std::vector<SWorkspaceCard>& cards, const WORKSPACEID id) const {
        for (size_t i = 0; i < cards.size(); ++i) {
            if (cards[i].id == id)
                return static_cast<int>(i);
        }

        return -1;
    }

    bool CWorkspaceRepository::cardIsActive(const SWorkspaceCard& card, const PHLMONITOR& monitor) const {
        if (card.special)
            return card.workspace && monitor->m_activeSpecialWorkspace == card.workspace;

        return card.id == activeNormalWorkspaceID(monitor);
    }

    bool CWorkspaceRepository::cardIsSelected(const SWorkspaceCard& card) const {
        if (card.special)
            return activePlugin()->selection().selectedRow() == ESelectedRow::SPECIAL && card.id == activePlugin()->selection().selectedSpecialID();

        return activePlugin()->selection().selectedRow() == ESelectedRow::NORMAL && card.id == activePlugin()->selection().selectedNormalID();
    }

} // namespace hyprdeck
