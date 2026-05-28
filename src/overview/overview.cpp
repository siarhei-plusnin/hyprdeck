#include "overview.hpp"

#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "rendering.hpp"
#include "shortcuts.hpp"
#include "runtime_types.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <helpers/Monitor.hpp>
#include <managers/PointerManager.hpp>
#include <render/Renderer.hpp>

#include <algorithm>
#include <utility>

namespace hyprdeck {
    namespace {

        void resetInteractionState() {
            activePlugin()->overviewPointer().resetState();
            activePlugin()->naming().resetPromptState();
            activePlugin()->workspaceFilter().resetPromptState();
            activePlugin()->confirmation().resetState();
            activePlugin()->shortcuts().resetState();
        }

        void resetSelectionState() {
            activePlugin()->selection().resetState();
        }

    } // namespace

    bool COverviewController::active() const {
        return m_session.active;
    }

    MONITORID COverviewController::monitorID() const {
        return m_session.monitorID;
    }

    PHLMONITOR COverviewController::monitor() const {
        if (m_session.monitorID == MONITOR_INVALID)
            return nullptr;

        return activePlugin()->hyprland().monitorFromID(m_session.monitorID);
    }

    double COverviewController::zoom() const {
        return m_session.zoom;
    }

    void COverviewController::setZoom(const double value) {
        m_session.zoom = std::clamp(value, MIN_ZOOM, MAX_ZOOM);
    }

    void COverviewController::close() {
        if (!m_session.active)
            return;

        const auto monitor = this->monitor();

        m_session.active    = false;
        m_session.monitorID = MONITOR_INVALID;
        activePlugin()->layout().clearCards();
        activePlugin()->layout().invalidate();
        resetInteractionState();
        resetSelectionState();

        if (monitor) {
            activePlugin()->hyprland().unlockSoftwarePointer(monitor);
            activePlugin()->hyprland().damageMonitor(monitor);
        }
    }

    void COverviewController::open() {
        const auto monitor = activePlugin()->hyprland().activeMonitor();
        if (!monitor)
            return;

        if (!m_session.zoomInitialized) {
            m_session.zoom            = activePlugin()->config().defaultZoom();
            m_session.zoomInitialized = true;
        }

        m_session.active               = true;
        activePlugin()->layout().setResetCamera(true);
        m_session.monitorID            = monitor->m_id;
        activePlugin()->selection().setActiveSelection(activePlugin()->workspaces().activeNormalWorkspaceID(monitor), activePlugin()->workspaces().activeSpecialWorkspaceID(monitor));
        resetInteractionState();
        activePlugin()->layout().invalidate();

        activePlugin()->hyprland().lockSoftwarePointer(monitor);

        activePlugin()->layout().recalculateCards(monitor);
        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void COverviewController::handleRenderStage(const eRenderStage stage) {
        if (!m_session.active || stage != RENDER_LAST_MOMENT)
            return;

        const auto renderMonitor = activePlugin()->hyprland().renderMonitor();
        if (!renderMonitor || renderMonitor->m_id != m_session.monitorID)
            return;

        const bool externalInputActive = activePlugin()->overlays().externalOverlayActive(renderMonitor) || activePlugin()->overlays().pointerOverNotificationOverlay(renderMonitor);
        if (externalInputActive) {
            if (activePlugin()->hyprland().softwarePointerLockedFor(renderMonitor))
                activePlugin()->hyprland().unlockSoftwarePointer(renderMonitor);
            activePlugin()->hyprland().setCursorHidden(false);
        } else if (!activePlugin()->hyprland().softwarePointerLockedFor(renderMonitor))
            activePlugin()->hyprland().lockSoftwarePointer(renderMonitor);

        activePlugin()->renderer().renderOverview(renderMonitor);
        if (!externalInputActive)
            activePlugin()->renderer().renderCursorOverlay(renderMonitor);

        const auto mode = activePlugin()->inputRouter().activeInputMode();
        if (mode == EInputMode::NAMING || mode == EInputMode::FILTER || mode == EInputMode::SHORTCUTS)
            activePlugin()->hyprland().damageMonitor(renderMonitor);
    }

    SDispatchResult COverviewController::toggle(std::string) {
        if (m_session.active)
            close();
        else
            open();

        return SDispatchResult{.passEvent = false, .success = true};
    }

    int COverviewController::luaToggle(lua_State*) {
        toggle("");
        return 0;
    }

} // namespace hyprdeck
