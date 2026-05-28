#include "navigation.hpp"

#include "layout.hpp"
#include "naming.hpp"
#include "plugin.hpp"
#include "selection.hpp"
#include "strings.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <desktop/Workspace.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>
#include <managers/EventManager.hpp>
#include <render/Renderer.hpp>

#include <string>

namespace hyprdeck {

    namespace {

        PHLWORKSPACE createSpecialWorkspace(const PHLMONITOR& monitor, const std::string& name = "") {
            const auto id = activePlugin()->hyprland().newSpecialWorkspaceID();
            if (!activePlugin()->hyprland().isSpecialWorkspaceID(id))
                return nullptr;

            const auto workspaceName = name.empty() ? "special:" + std::to_string(-id) : "special:" + name;
            return activePlugin()->hyprland().createWorkspace(id, monitor->m_id, workspaceName, true);
        }

        void openSpecialWorkspace(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor, const bool selectSpecial = true) {
            if (!workspace)
                return;

            activePlugin()->selection().setSelectedSpecialID(workspace->m_id);
            activePlugin()->naming().resetPromptState();

            if (selectSpecial)
                activePlugin()->selection().setSelectedRow(ESelectedRow::SPECIAL);
            else {
                activePlugin()->selection().setSelectedRow(ESelectedRow::NORMAL);
                activePlugin()->selection().setSelectedNormalID(activePlugin()->workspaces().activeNormalWorkspaceID(monitor));
            }

            monitor->setSpecialWorkspace(workspace);
            activePlugin()->layout().invalidate();
            activePlugin()->layout().recalculateCards(monitor);
            activePlugin()->layout().centerSpecialCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), workspace->m_id));
            activePlugin()->hyprland().damageMonitor(monitor);
        }

        bool createNamedSpecialWorkspaceInternal(const std::string& name, const PHLMONITOR& monitor) {
            if (activePlugin()->workspaceFilter().applied())
                return false;

            const auto normalizedName = strings::normalizeSpecialWorkspaceName(name);
            if (normalizedName.empty())
                return false;

            auto workspace = activePlugin()->hyprland().workspaceByName("special:" + normalizedName);
            if (!workspace)
                workspace = createSpecialWorkspace(monitor, normalizedName);

            if (!workspace)
                return false;

            openSpecialWorkspace(workspace, monitor);
            return true;
        }

        bool renameSelectedSpecialWorkspaceInternal(const std::string& name, const PHLMONITOR& monitor) {
            const auto normalizedName = strings::normalizeSpecialWorkspaceName(name);
            if (normalizedName.empty())
                return false;

            const auto* card = activePlugin()->selection().selectedSpecialCard();
            if (!card || !card->workspace)
                return false;

            const auto workspace = card->workspace;
            workspace->m_name    = "special:" + normalizedName;

            activePlugin()->hyprland().postWorkspaceRenameEvent(workspace);

            workspace->m_events.renamed.emit();

            activePlugin()->selection().setSelectedRow(ESelectedRow::SPECIAL);
            activePlugin()->selection().setSelectedSpecialID(workspace->m_id);
            activePlugin()->naming().resetPromptState();
            activePlugin()->layout().invalidate();
            activePlugin()->layout().recalculateCards(monitor);
            activePlugin()->hyprland().damageMonitor(monitor);
            return true;
        }

        std::vector<PHLWINDOW> workspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor = nullptr) {
            std::vector<PHLWINDOW> windows;
            for (const auto& window : activePlugin()->hyprland().windows()) {
                if (activePlugin()->workspaces().windowBelongsToWorkspace(window, workspace) && (!monitor || activePlugin()->workspaces().windowBelongsToMonitor(window, monitor)))
                    windows.push_back(window);
            }

            return windows;
        }

        void closeWindows(const std::vector<PHLWINDOW>& windows) {
            for (const auto& window : windows)
                window->sendClose();
        }

    } // namespace

    void CWorkspaceNavigator::closeWorkspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
        closeWindows(workspaceWindows(workspace, monitor));
    }

    bool CWorkspaceNavigator::switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor) {
        auto workspace = card.workspace;

        if (!workspace && !card.special) {
            if (activePlugin()->workspaceFilter().applied())
                return false;

            workspace = activePlugin()->hyprland().createWorkspace(card.id, monitor->m_id, std::to_string(card.id), true);
        }

        if (!workspace)
            return false;

        if (card.special) {
            activePlugin()->selection().setSelectedRow(ESelectedRow::SPECIAL);
            activePlugin()->selection().setSelectedSpecialID(workspace->m_id);

            if (monitor->m_activeSpecialWorkspace == workspace)
                monitor->setSpecialWorkspace(nullptr);
            else
                monitor->setSpecialWorkspace(workspace);
        } else {
            activePlugin()->selection().setSelectedRow(ESelectedRow::NORMAL);
            activePlugin()->selection().setSelectedNormalID(workspace->m_id);

            monitor->changeWorkspace(workspace, false, true, false);
        }

        activePlugin()->layout().invalidate();
        activePlugin()->hyprland().damageMonitor(monitor);

        return true;
    }

    bool CWorkspaceNavigator::createNamedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor) {
        return createNamedSpecialWorkspaceInternal(name, monitor);
    }

    bool CWorkspaceNavigator::renameSelectedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor) {
        return renameSelectedSpecialWorkspaceInternal(name, monitor);
    }

    void CWorkspaceNavigator::createSimpleSpecialWorkspace(const PHLMONITOR& monitor) {
        if (activePlugin()->workspaceFilter().applied())
            return;

        activePlugin()->naming().resetPromptState();

        openSpecialWorkspace(createSpecialWorkspace(monitor), monitor);
    }

} // namespace hyprdeck
