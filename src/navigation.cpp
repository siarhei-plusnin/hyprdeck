#include "navigation.hpp"

#include "layout.hpp"
#include "naming.hpp"
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

        PHLWORKSPACE          createSpecialWorkspace(const PHLMONITOR& monitor, const std::string& name = "") {
            const auto id = g_pCompositor->getNewSpecialID();
            if (!monitor || !g_pCompositor->isWorkspaceSpecial(id))
                return nullptr;

            const auto workspaceName = name.empty() ? "special:" + std::to_string(-id) : "special:" + name;
            auto       workspace     = g_pCompositor->createNewWorkspace(id, monitor->m_id, workspaceName, true);
            if (workspace)
                state().selection.pendingSpecialID = workspace->m_id;

            return workspace;
        }

        void openSpecialWorkspace(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor, const bool selectSpecial = true) {
            if (!workspace || !monitor)
                return;

            auto& selection             = state().selection;
            selection.selectedSpecialID = workspace->m_id;
            resetNamingPromptState();

            if (selectSpecial)
                selection.selectedRow = ESelectedRow::SPECIAL;
            else {
                selection.selectedRow      = ESelectedRow::NORMAL;
                selection.selectedNormalID = activeNormalWorkspaceID(monitor);
            }

            monitor->setSpecialWorkspace(workspace);
            invalidateLayout();
            recalculateCards(monitor);
            centerSpecialCard(cardIndexByID(state().layout.specialCards, workspace->m_id));
            g_pHyprRenderer->damageMonitor(monitor);
        }

        bool createNamedSpecialWorkspaceInternal(const std::string& name, const PHLMONITOR& monitor) {
            if (workspaceFilterApplied())
                return false;

            const auto normalizedName = strings::normalizeSpecialWorkspaceName(name);
            if (normalizedName.empty())
                return false;

            cleanupPendingSpecialWorkspace(monitor);

            auto workspace = g_pCompositor->getWorkspaceByName("special:" + normalizedName);
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

            const auto* card = selectedSpecialCard();
            if (!card || !card->workspace)
                return false;

            auto&      current   = state();
            auto&      selection = current.selection;
            const auto workspace = card->workspace;
            workspace->m_name    = "special:" + normalizedName;

            if (g_pEventManager)
                g_pEventManager->postEvent({.event = "renameworkspace", .data = std::to_string(workspace->m_id) + "," + workspace->m_name});

            workspace->m_events.renamed.emit();

            selection.selectedRow       = ESelectedRow::SPECIAL;
            selection.selectedSpecialID = workspace->m_id;
            resetNamingPromptState();
            invalidateLayout();
            recalculateCards(monitor);
            g_pHyprRenderer->damageMonitor(monitor);
            return true;
        }

        std::vector<PHLWINDOW> workspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor = nullptr) {
            std::vector<PHLWINDOW> windows;
            for (const auto& window : g_pCompositor->m_windows) {
                if (windowBelongsToWorkspace(window, workspace) && (!monitor || windowBelongsToMonitor(window, monitor)))
                    windows.push_back(window);
            }

            return windows;
        }

        void closeWindows(const std::vector<PHLWINDOW>& windows) {
            for (const auto& window : windows)
                window->sendClose();
        }

    } // namespace

    bool cleanupPendingSpecialWorkspace(const PHLMONITOR& monitor) {
        auto& selection = state().selection;
        if (selection.pendingSpecialID == WORKSPACE_INVALID)
            return false;

        const auto pendingID     = selection.pendingSpecialID;
        const auto workspace     = g_pCompositor->getWorkspaceByID(pendingID);
        selection.pendingSpecialID = WORKSPACE_INVALID;
        invalidateLayout();

        if (!workspace || !workspace->m_isSpecialWorkspace)
            return false;

        if (workspaceHasAnyWindows(workspace, monitor))
            return false;

        if (monitor && monitor->m_activeSpecialWorkspace == workspace)
            monitor->setSpecialWorkspace(nullptr);

        if (selection.selectedSpecialID == pendingID)
            selection.selectedSpecialID = WORKSPACE_INVALID;

        return true;
    }

    void closeWorkspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) {
        closeWindows(workspaceWindows(workspace, monitor));
    }

    bool switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor) {
        auto workspace = card.workspace;

        if (!workspace && !card.special) {
            if (workspaceFilterApplied())
                return false;

            workspace = g_pCompositor->createNewWorkspace(card.id, monitor->m_id, std::to_string(card.id), true);
        }

        if (!workspace)
            return false;

        auto& current = state();
        auto& selection = current.selection;

        if (card.special) {
            selection.selectedRow       = ESelectedRow::SPECIAL;
            selection.selectedSpecialID = workspace->m_id;

            if (monitor->m_activeSpecialWorkspace == workspace)
                monitor->setSpecialWorkspace(nullptr);
            else
                monitor->setSpecialWorkspace(workspace);
        } else {
            selection.selectedRow      = ESelectedRow::NORMAL;
            selection.selectedNormalID = workspace->m_id;

            monitor->changeWorkspace(workspace, false, true, false);
        }

        invalidateLayout();
        g_pHyprRenderer->damageMonitor(monitor);

        return true;
    }

    bool createNamedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor) {
        return createNamedSpecialWorkspaceInternal(name, monitor);
    }

    bool renameSelectedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor) {
        return renameSelectedSpecialWorkspaceInternal(name, monitor);
    }

    void createSimpleSpecialWorkspace(const PHLMONITOR& monitor) {
        if (workspaceFilterApplied())
            return;

        cleanupPendingSpecialWorkspace(monitor);

        resetNamingPromptState();

        openSpecialWorkspace(createSpecialWorkspace(monitor), monitor);
    }

} // namespace hyprdeck
