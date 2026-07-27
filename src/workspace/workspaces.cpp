#include "workspaces.hpp"
#include "SharedDefs.hpp"
#include "macros.hpp"
#include "plugin.hpp"
#include "strings.hpp"

#include <algorithm>
#include <desktop/Workspace.hpp>
#include <desktop/view/Window.hpp>
#include <output/Monitor.hpp>

#include <string>

namespace hyprdeck {
    bool CWorkspaceRepository::isNormalWorkspace(const PHLWORKSPACE& workspace) const {
        return workspace && !workspace->m_isSpecialWorkspace && workspace->m_id != WORKSPACE_INVALID;
    }

    bool CWorkspaceRepository::isNumericNormalWorkspace(const PHLWORKSPACE& workspace) const {
        return isNormalWorkspace(workspace) && workspace->m_id > 0;
    }

    bool CWorkspaceRepository::isNamedNormalWorkspace(const PHLWORKSPACE& workspace) const {
        return isNormalWorkspace(workspace) && workspace->m_id <= 0;
    }

    std::string CWorkspaceRepository::normalWorkspaceLabel(const PHLWORKSPACE& workspace) const {
        if (!isNormalWorkspace(workspace))
            return "";

        if (isNamedNormalWorkspace(workspace))
            return workspace->m_name;

        const auto id = std::to_string(workspace->m_id);
        return workspace->m_name.empty() || workspace->m_name == id ? id : id + ": " + workspace->m_name;
    }

    std::vector<PHLWORKSPACE> CWorkspaceRepository::namedNormalWorkspacesToShow() const {
        std::vector<PHLWORKSPACE> workspaces;
        for (const auto& workspace : activePlugin()->hyprland().workspacesCopy()) {
            if (isNamedNormalWorkspace(workspace))
                workspaces.push_back(workspace);
        }

        std::ranges::sort(workspaces, [](const auto& lhs, const auto& rhs) { return strings::lower(lhs->m_name) < strings::lower(rhs->m_name); });
        return workspaces;
    }

    PHLMONITOR CWorkspaceRepository::workspaceMonitor(const PHLWORKSPACE& workspace) const {
        if (!workspace)
            return nullptr;

        if (const auto monitor = workspace->m_monitor.lock(); monitor)
            return monitor;

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (windowBelongsToWorkspace(window, workspace)) {
                if (const auto monitor = window->m_monitor.lock(); monitor)
                    return monitor;
            }
        }

        for (const auto& monitor : activePlugin()->hyprland().monitors()) {
            if (monitor && (monitor->m_activeWorkspace == workspace || monitor->m_activeSpecialWorkspace == workspace))
                return monitor;
        }

        return nullptr;
    }

    bool CWorkspaceRepository::workspaceBelongsToMonitor(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const {
        const auto owner = workspaceMonitor(workspace);
        return owner && monitor && owner->m_id == monitor->m_id;
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
            if (!window || !window->m_isMapped || window->isHidden())
                continue;

            if (windowBelongsToMonitor(window, monitor) && windowBelongsToWorkspace(window, workspace))
                return true;
        }

        return false;
    }

    bool CWorkspaceRepository::workspaceHasAnyWindows(const PHLWORKSPACE& workspace) const {
        if (!workspace)
            return false;

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (window && window->m_isMapped && !window->isHidden() && windowBelongsToWorkspace(window, workspace))
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

    std::vector<PHLWORKSPACE> CWorkspaceRepository::specialWorkspacesToShow() const {
        std::vector<PHLWORKSPACE> workspaces;
        for (const auto& workspace : activePlugin()->hyprland().workspacesCopy()) {
            if (!workspace || !workspace->m_isSpecialWorkspace)
                continue;

            const bool active =
                std::ranges::any_of(activePlugin()->hyprland().monitors(), [&](const auto& monitor) { return monitor && monitor->m_activeSpecialWorkspace == workspace; });
            if (!workspaceHasAnyWindows(workspace) && !active)
                continue;

            workspaces.push_back(workspace);
        }

        std::ranges::sort(workspaces, [this](const auto& lhs, const auto& rhs) { return specialWorkspaceId(lhs) < specialWorkspaceId(rhs); });
        return workspaces;
    }

    WORKSPACEID CWorkspaceRepository::lastWorkspaceToShow() const {
        WORKSPACEID highestExisting = 0;
        for (const auto& workspace : activePlugin()->hyprland().workspacesCopy()) {
            if (isNumericNormalWorkspace(workspace))
                highestExisting = std::max(highestExisting, workspace->m_id);
        }

        return std::max<WORKSPACEID>(3, highestExisting + 1);
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
        const auto owner = workspaceMonitor(card.workspace);
        if (!owner)
            return false;

        if (card.special)
            return owner->m_activeSpecialWorkspace == card.workspace;

        return owner->m_activeWorkspace == card.workspace;
    }

    bool CWorkspaceRepository::cardIsNative(const SWorkspaceCard& card, const PHLMONITOR& monitor) const {
        if (!monitor)
            return false;
        if (!card.workspace)
            return !card.special;

        return workspaceBelongsToMonitor(card.workspace, monitor);
    }

    bool CWorkspaceRepository::cardIsSelected(const SWorkspaceCard& card) const {
        if (card.special)
            return activePlugin()->selection().selectedRow() == ESelectedRow::SPECIAL && card.id == activePlugin()->selection().selectedSpecialID();

        return activePlugin()->selection().selectedRow() == ESelectedRow::NORMAL && card.id == activePlugin()->selection().selectedNormalID();
    }

} // namespace hyprdeck
