#include "input.hpp"

#include "confirmation.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "overview_controller.hpp"
#include "overview_interaction.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "shortcuts.hpp"
#include "runtime_types.hpp"
#include "workspace_filter.hpp"

namespace hyprdeck {
    PHLMONITOR CInputRouter::activeHostMonitor() const {
        if (!activePlugin()->overview().active())
            return nullptr;

        const auto monitor = activePlugin()->overview().hostMonitor();
        if (!monitor)
            activePlugin()->overview().close(true);

        return monitor;
    }

    PHLMONITOR CInputRouter::activeSelectedMonitor() const {
        if (!activePlugin()->overview().active())
            return nullptr;

        if (const auto monitor = activePlugin()->overview().selectedMonitor(); monitor)
            return monitor;

        const auto host = activePlugin()->overview().hostMonitor();
        activePlugin()->overview().selectMonitor(host);
        return activePlugin()->overview().selectedMonitor();
    }

    bool CInputRouter::inputBlockedByExternalOverlay(const PHLMONITOR& monitor) const {
        return activePlugin()->overlays().externalOverlayActive(monitor);
    }

    EInputMode CInputRouter::activeInputMode() const {
        if (!activePlugin()->overview().active())
            return EInputMode::INACTIVE;

        if (activePlugin()->shortcuts().menuOpen())
            return EInputMode::SHORTCUTS;

        if (activePlugin()->naming().promptOpen())
            return EInputMode::NAMING;

        if (activePlugin()->workspaceFilter().promptOpen())
            return EInputMode::FILTER;

        if (activePlugin()->confirmation().promptOpen())
            return EInputMode::CONFIRMATION;

        return EInputMode::OVERVIEW;
    }

    void CInputRouter::handleMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        const auto monitor = activeHostMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseMove(position, info, monitor);
    }

    void CInputRouter::handleMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeHostMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseButton(event, info, monitor);
    }

    void CInputRouter::handleMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeHostMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseAxis(event, info, monitor);
    }

    void CInputRouter::handleKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
        const auto hostMonitor     = activeHostMonitor();
        const auto selectedMonitor = activeSelectedMonitor();
        if (!hostMonitor || !selectedMonitor || inputBlockedByExternalOverlay(hostMonitor))
            return;

        const auto mode = activeInputMode();
        if (mode == EInputMode::INACTIVE)
            return;

        if (mode == EInputMode::SHORTCUTS) {
            info.cancelled = true;
            activePlugin()->shortcuts().handleKey(event, hostMonitor);
            return;
        }

        if (activePlugin()->shortcuts().isMenuKey(event)) {
            info.cancelled = true;
            activePlugin()->shortcuts().openMenu(hostMonitor);
            return;
        }

        if (mode == EInputMode::NAMING) {
            info.cancelled = true;
            activePlugin()->naming().handleKey(event, selectedMonitor);
            return;
        }

        if (mode == EInputMode::FILTER) {
            info.cancelled = true;
            activePlugin()->workspaceFilter().handleKey(event, selectedMonitor);
            return;
        }

        if (mode == EInputMode::CONFIRMATION) {
            info.cancelled = true;
            activePlugin()->confirmation().handleKey(event, selectedMonitor);
            return;
        }

        if (activePlugin()->overviewKeyboard().handleKey(event, selectedMonitor))
            info.cancelled = true;
    }

} // namespace hyprdeck
