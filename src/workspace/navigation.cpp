#include "navigation.hpp"

#include "plugin.hpp"
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

        SWorkspaceNavigationResult openSpecialWorkspace(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor, const bool selectSpecial = true) {
            if (!workspace)
                return {};

            SWorkspaceNavigationResult result{
                .success           = true,
                .selectedSpecialID = workspace->m_id,
            };

            if (selectSpecial) {
                result.selectedRow = ESelectedRow::SPECIAL;
            } else {
                result.selectedRow      = ESelectedRow::NORMAL;
                result.selectedNormalID = activePlugin()->workspaces().activeNormalWorkspaceID(monitor);
            }

            monitor->setSpecialWorkspace(workspace);
            return result;
        }

        SWorkspaceNavigationResult createNamedSpecialWorkspaceInternal(const std::string& name, const PHLMONITOR& monitor) {
            if (activePlugin()->workspaceFilter().applied())
                return {};

            const auto normalizedName = strings::normalizeSpecialWorkspaceName(name);
            if (normalizedName.empty())
                return {};

            auto workspace = activePlugin()->hyprland().workspaceByName("special:" + normalizedName);
            if (!workspace)
                workspace = createSpecialWorkspace(monitor, normalizedName);

            if (!workspace)
                return {};

            return openSpecialWorkspace(workspace, monitor);
        }

        SWorkspaceNavigationResult renameSpecialWorkspaceInternal(const PHLWORKSPACE& workspace, const std::string& name) {
            const auto normalizedName = strings::normalizeSpecialWorkspaceName(name);
            if (normalizedName.empty())
                return {};

            if (!workspace)
                return {};

            workspace->m_name    = "special:" + normalizedName;

            activePlugin()->hyprland().postWorkspaceRenameEvent(workspace);

            workspace->m_events.renamed.emit();

            return SWorkspaceNavigationResult{
                .success           = true,
                .selectedRow       = ESelectedRow::SPECIAL,
                .selectedSpecialID = workspace->m_id,
            };
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

    SWorkspaceNavigationResult CWorkspaceNavigator::switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor) {
        auto workspace = card.workspace;

        if (!workspace && !card.special) {
            if (activePlugin()->workspaceFilter().applied())
                return {};

            workspace = activePlugin()->hyprland().createWorkspace(card.id, monitor->m_id, std::to_string(card.id), true);
        }

        if (!workspace)
            return {};

        SWorkspaceNavigationResult result{
            .success = true,
        };

        if (card.special) {
            result.selectedRow       = ESelectedRow::SPECIAL;
            result.selectedSpecialID = workspace->m_id;

            if (monitor->m_activeSpecialWorkspace == workspace)
                monitor->setSpecialWorkspace(nullptr);
            else
                monitor->setSpecialWorkspace(workspace);
        } else {
            result.selectedRow      = ESelectedRow::NORMAL;
            result.selectedNormalID = workspace->m_id;

            monitor->changeWorkspace(workspace, false, true, false);
        }

        return result;
    }

    SWorkspaceNavigationResult CWorkspaceNavigator::createNamedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor) {
        return createNamedSpecialWorkspaceInternal(name, monitor);
    }

    SWorkspaceNavigationResult CWorkspaceNavigator::renameSpecialWorkspace(const PHLWORKSPACE& workspace, const std::string& name) {
        return renameSpecialWorkspaceInternal(workspace, name);
    }

    SWorkspaceNavigationResult CWorkspaceNavigator::createSimpleSpecialWorkspace(const PHLMONITOR& monitor) {
        if (activePlugin()->workspaceFilter().applied())
            return {};

        return openSpecialWorkspace(createSpecialWorkspace(monitor), monitor);
    }

} // namespace hyprdeck
