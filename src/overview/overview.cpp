#include "overview.hpp"

#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "plugin.hpp"
#include "rendering.hpp"
#include "shortcuts.hpp"
#include "runtime_types.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <output/Monitor.hpp>
#include <render/Renderer.hpp>

#include <algorithm>

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

    bool COverviewController::rendering() const {
        return m_session.rendering;
    }

    MONITORID COverviewController::hostMonitorID() const {
        return m_session.hostMonitorID;
    }

    MONITORID COverviewController::selectedMonitorID() const {
        return m_session.selectedMonitorID;
    }

    PHLMONITOR COverviewController::hostMonitor() const {
        if (m_session.hostMonitorID == MONITOR_INVALID)
            return nullptr;

        return activePlugin()->hyprland().monitorFromID(m_session.hostMonitorID);
    }

    PHLMONITOR COverviewController::selectedMonitor() const {
        if (m_session.selectedMonitorID == MONITOR_INVALID)
            return nullptr;

        return activePlugin()->hyprland().monitorFromID(m_session.selectedMonitorID);
    }

    double COverviewController::zoom() const {
        return m_session.zoom;
    }

    void COverviewController::setZoom(const double value) {
        m_session.zoom = std::clamp(value, MIN_ZOOM, MAX_ZOOM);
    }

    void COverviewController::damageHost() const {
        activePlugin()->hyprland().damageMonitor(hostMonitor());
    }

    void COverviewController::setPointerLocked(const PHLMONITOR& monitor, const bool locked) {
        if (locked) {
            if (!monitor)
                return;
            if (m_session.pointerLockMonitorID == monitor->m_id && activePlugin()->hyprland().softwarePointerLockedFor(monitor))
                return;

            if (m_session.pointerLockMonitorID != MONITOR_INVALID)
                setPointerLocked(activePlugin()->hyprland().monitorFromID(m_session.pointerLockMonitorID), false);

            activePlugin()->hyprland().lockSoftwarePointer(monitor);
            m_session.pointerLockMonitorID = monitor->m_id;
            return;
        }

        if (m_session.pointerLockMonitorID == MONITOR_INVALID)
            return;

        const auto lockedMonitor = monitor && monitor->m_id == m_session.pointerLockMonitorID ? monitor : activePlugin()->hyprland().monitorFromID(m_session.pointerLockMonitorID);
        if (lockedMonitor)
            activePlugin()->hyprland().unlockSoftwarePointer(lockedMonitor);
        m_session.pointerLockMonitorID = MONITOR_INVALID;
    }

    void COverviewController::finishClose(const PHLMONITOR& monitor) {
        m_session.active               = false;
        m_session.rendering            = false;
        m_session.closing              = false;
        m_session.hostMonitorID        = MONITOR_INVALID;
        m_session.selectedMonitorID    = MONITOR_INVALID;
        m_session.pointerLockMonitorID = MONITOR_INVALID;
        activePlugin()->layout().clearCards();
        activePlugin()->layout().invalidate();
        resetSelectionState();

        if (monitor)
            activePlugin()->hyprland().damageMonitor(monitor);
    }

    bool COverviewController::prepareWorkspaceCommit() {
        if (!m_session.active || !m_session.rendering)
            return false;

        // Stop intercepting Hyprland's focus-induced pointer update, but keep the
        // host software cursor locked until the output handoff is complete.
        m_session.active = false;
        resetInteractionState();
        return true;
    }

    void COverviewController::close(const bool instant) {
        if (!m_session.rendering)
            return;

        const auto monitor = hostMonitor();

        m_session.active  = false;
        m_session.closing = true;
        resetInteractionState();

        if (monitor) {
            // Restore while still software-locked so the backend cannot start a nested cursor render.
            activePlugin()->hyprland().setCursorHidden(false);
            setPointerLocked(monitor, false);
            activePlugin()->hyprland().damageMonitor(monitor);
            activePlugin()->hyprland().scheduleAnimationFrame(monitor);
        }

        if (instant || !monitor || !activePlugin()->animations().startOverviewClose(monitor))
            finishClose(monitor);
    }

    void COverviewController::open() {
        const auto monitor = activePlugin()->hyprland().activeMonitor();
        if (!monitor)
            return;

        openOn(monitor);
    }

    void COverviewController::openOn(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        if (m_session.rendering)
            close(true);

        activePlugin()->animations().reset();

        if (!m_session.zoomInitialized) {
            m_session.zoom            = activePlugin()->config().defaultZoom();
            m_session.zoomInitialized = true;
        }

        m_session.active    = true;
        m_session.rendering = true;
        m_session.closing   = false;
        activePlugin()->layout().setResetCamera(true);
        m_session.hostMonitorID     = monitor->m_id;
        m_session.selectedMonitorID = monitor->m_id;
        activePlugin()->selection().setActiveSelection(activePlugin()->workspaces().activeNormalWorkspaceID(monitor),
                                                       activePlugin()->workspaces().activeSpecialWorkspaceID(monitor));
        resetInteractionState();
        activePlugin()->layout().invalidate();

        setPointerLocked(monitor, true);
        activePlugin()->hyprland().setCursorHidden(false);

        activePlugin()->layout().recalculateCards(monitor);
        activePlugin()->animations().startOverviewOpen(monitor);
        activePlugin()->hyprland().damageMonitor(monitor);
        activePlugin()->hyprland().scheduleAnimationFrame(monitor);
    }

    void COverviewController::selectMonitor(const PHLMONITOR& monitor) {
        if (!m_session.active || !monitor || monitor->m_id == m_session.selectedMonitorID)
            return;

        const auto monitors = activePlugin()->hyprland().monitors();
        const auto found    = std::ranges::find_if(monitors, [&](const auto& candidate) { return candidate && candidate->m_id == monitor->m_id; });
        if (found == monitors.end())
            return;

        activePlugin()->animations().reset();
        activePlugin()->overviewPointer().resetState();
        m_session.selectedMonitorID = monitor->m_id;
        activePlugin()->selection().setLastActiveNormalID(activePlugin()->workspaces().activeNormalWorkspaceID(monitor));
        activePlugin()->selection().setLastActiveSpecialID(activePlugin()->workspaces().activeSpecialWorkspaceID(monitor));
        activePlugin()->layout().setResetCamera(false);
        activePlugin()->layout().invalidate();
        activePlugin()->layout().recalculateCards(monitor);

        if (activePlugin()->selection().selectedRow() == ESelectedRow::SPECIAL)
            activePlugin()->layout().centerSpecialCard(
                activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().specialCards(), activePlugin()->selection().selectedSpecialID()));
        else
            activePlugin()->layout().centerNormalCard(activePlugin()->workspaces().cardIndexByID(activePlugin()->layout().cards(), activePlugin()->selection().selectedNormalID()));

        damageHost();
    }

    void COverviewController::cycleMonitor(const int direction) {
        const auto monitors = activePlugin()->hyprland().monitors();
        if (monitors.size() < 2)
            return;

        const auto current = std::ranges::find_if(monitors, [&](const auto& monitor) { return monitor && monitor->m_id == m_session.selectedMonitorID; });
        const auto index   = current == monitors.end() ? 0 : static_cast<int>(std::distance(monitors.begin(), current));
        const auto count   = static_cast<int>(monitors.size());
        selectMonitor(monitors[(index + (direction < 0 ? -1 : 1) + count) % count]);
    }

    void COverviewController::handleStateChanged() {
        if (!m_session.rendering)
            return;

        const auto host = hostMonitor();
        if (!host) {
            finishClose(nullptr);
            return;
        }

        if (!selectedMonitor())
            selectMonitor(host);

        activePlugin()->layout().invalidate();
        activePlugin()->hyprland().damageMonitor(host);
    }

    void COverviewController::handleMonitorRemoved(const PHLMONITOR& monitor) {
        if (!monitor || !m_session.rendering)
            return;

        if (monitor->m_id == m_session.hostMonitorID) {
            activePlugin()->hyprland().setCursorHidden(false);
            setPointerLocked(monitor, false);
            resetInteractionState();
            activePlugin()->animations().reset();
            finishClose(monitor);
            return;
        }

        if (monitor->m_id != m_session.selectedMonitorID)
            return;

        resetInteractionState();
        const auto host = hostMonitor();
        if (host)
            selectMonitor(host);
        else if (const auto monitors = activePlugin()->hyprland().monitors(); !monitors.empty())
            selectMonitor(monitors.front());
    }

    void COverviewController::handleRenderStage(const eRenderStage stage) {
        if (!m_session.rendering || stage != RENDER_LAST_MOMENT)
            return;

        const auto renderMonitor = activePlugin()->hyprland().renderMonitor();
        const auto host          = hostMonitor();
        if (!renderMonitor || !host)
            return;

        if (renderMonitor->m_id != host->m_id) {
            if (m_session.active)
                activePlugin()->hyprland().damageMonitor(host);
            return;
        }

        activePlugin()->animations().update(renderMonitor);
        activePlugin()->renderer().renderOverview(renderMonitor);
        if (m_session.active)
            activePlugin()->renderer().renderCursorOverlay(renderMonitor);

        if (m_session.closing && !activePlugin()->animations().overviewAnimating()) {
            finishClose(renderMonitor);
            return;
        }

        const auto mode = activePlugin()->inputRouter().activeInputMode();
        if (activePlugin()->animations().active()) {
            activePlugin()->hyprland().damageMonitor(renderMonitor);
            activePlugin()->hyprland().scheduleAnimationFrame(renderMonitor);
        } else if (mode == EInputMode::NAMING || mode == EInputMode::FILTER || mode == EInputMode::SHORTCUTS)
            activePlugin()->hyprland().damageMonitor(renderMonitor);
    }

    SDispatchResult COverviewController::toggle(std::string) {
        const auto requestedMonitor = activePlugin()->hyprland().activeMonitor();
        if (m_session.active && requestedMonitor && requestedMonitor->m_id == m_session.hostMonitorID)
            close();
        else if (requestedMonitor)
            openOn(requestedMonitor);

        return SDispatchResult{.passEvent = false, .success = true};
    }

    int COverviewController::luaToggle(lua_State*) {
        toggle("");
        return 0;
    }

} // namespace hyprdeck
