#include "navigation.hpp"

#include "plugin.hpp"
#include "strings.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <desktop/Workspace.hpp>
#include <desktop/view/Window.hpp>
#include <output/Monitor.hpp>
#include <managers/EventManager.hpp>
#include <render/Renderer.hpp>

#include <algorithm>
#include <string>
#include <vector>

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

            const auto owner = activePlugin()->workspaces().workspaceMonitor(workspace);
            if (owner && owner->m_id != monitor->m_id)
                activePlugin()->hyprland().moveWorkspaceToMonitor(workspace, monitor);

            monitor->setSpecialWorkspace(workspace);
            activePlugin()->hyprland().focusMonitor(activePlugin()->overview().hostMonitor());
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

            workspace->m_name = "special:" + normalizedName;

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

        void animateSpecialCloseIfEmpty(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
            if (!workspace || activePlugin()->workspaces().workspaceHasAnyWindows(workspace, monitor))
                return;

            const auto& cards = activePlugin()->layout().specialCards();
            const int   index = activePlugin()->workspaces().cardIndexByID(cards, workspace->m_id);
            if (index >= 0)
                activePlugin()->animations().startSpecialCardClose(cards[index], activePlugin()->overview().hostMonitor());
        }

        WORKSPACEID nextSpecialSelectionAfterRemoving(const PHLWORKSPACE& workspace) {
            if (!workspace)
                return WORKSPACE_INVALID;

            const auto&              cards    = activePlugin()->layout().specialCards();
            int                      oldIndex = -1;
            std::vector<WORKSPACEID> remainingIDs;
            remainingIDs.reserve(cards.size());

            for (size_t i = 0; i < cards.size(); ++i) {
                if (cards[i].id == workspace->m_id) {
                    oldIndex = static_cast<int>(i);
                    continue;
                }

                remainingIDs.push_back(cards[i].id);
            }

            if (remainingIDs.empty())
                return WORKSPACE_INVALID;

            const auto nextIndex = static_cast<size_t>(std::clamp(oldIndex <= 0 ? 0 : oldIndex - 1, 0, static_cast<int>(remainingIDs.size() - 1)));
            return remainingIDs[nextIndex];
        }

    } // namespace

    void CWorkspaceNavigator::closeWorkspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
        closeWindows(workspaceWindows(workspace, monitor));
    }

    SWorkspaceNavigationResult CWorkspaceNavigator::hideActiveSpecialWorkspace(const PHLMONITOR& monitor, const bool animateIfEmpty) {
        if (!monitor || !monitor->m_activeSpecialWorkspace)
            return {};

        const auto                 workspace     = monitor->m_activeSpecialWorkspace;
        const bool                 willDisappear = !activePlugin()->workspaces().workspaceHasAnyWindows(workspace, monitor);

        SWorkspaceNavigationResult result{
            .success = true,
        };

        if (willDisappear) {
            result.selectedSpecialID = nextSpecialSelectionAfterRemoving(workspace);
            if (*result.selectedSpecialID == WORKSPACE_INVALID)
                result.selectedRow = ESelectedRow::NORMAL;
        } else
            result.selectedSpecialID = workspace->m_id;

        if (animateIfEmpty && willDisappear)
            animateSpecialCloseIfEmpty(workspace, monitor);

        monitor->setSpecialWorkspace(nullptr);
        activePlugin()->hyprland().focusMonitor(activePlugin()->overview().hostMonitor());
        return result;
    }

    SWorkspaceNavigationResult CWorkspaceNavigator::switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor, const bool focusTarget) {
        auto       workspace                = card.workspace;
        const bool specialWasActiveOnTarget = card.special && workspace && monitor && monitor->m_activeSpecialWorkspace == workspace;

        if (!workspace && !card.special) {
            if (activePlugin()->workspaceFilter().applied())
                return {};

            workspace = activePlugin()->hyprland().createWorkspace(card.id, monitor->m_id, std::to_string(card.id), true);
        }

        if (!workspace)
            return {};

        const auto owner = activePlugin()->workspaces().workspaceMonitor(workspace);
        if (owner && owner->m_id != monitor->m_id)
            activePlugin()->hyprland().moveWorkspaceToMonitor(workspace, monitor);

        SWorkspaceNavigationResult result{
            .success = true,
        };

        if (card.special) {
            result.selectedRow       = ESelectedRow::SPECIAL;
            result.selectedSpecialID = workspace->m_id;

            if (specialWasActiveOnTarget) {
                result = hideActiveSpecialWorkspace(monitor);
                if (!result.selectedRow && result.selectedSpecialID && *result.selectedSpecialID != WORKSPACE_INVALID)
                    result.selectedRow = ESelectedRow::SPECIAL;
            } else
                monitor->setSpecialWorkspace(workspace);
        } else {
            result.selectedRow      = ESelectedRow::NORMAL;
            result.selectedNormalID = workspace->m_id;

            monitor->changeWorkspace(workspace, false, true, !focusTarget);
        }

        activePlugin()->hyprland().focusMonitor(focusTarget ? monitor : activePlugin()->overview().hostMonitor());

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
