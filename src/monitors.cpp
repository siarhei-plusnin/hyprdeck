#include "monitors.hpp"

#include "state.hpp"

#include <Compositor.hpp>
#include <desktop/state/FocusState.hpp>

namespace hyprdeck {

    PHLMONITOR overviewMonitor() {
        const auto& session = state().session;
        if (session.monitorID == MONITOR_INVALID)
            return nullptr;

        return g_pCompositor->getMonitorFromID(session.monitorID);
    }

    PHLMONITOR activeMonitor() {
        if (const auto focusMonitor = Desktop::focusState() ? Desktop::focusState()->monitor() : nullptr; focusMonitor)
            return focusMonitor;

        return g_pCompositor->getMonitorFromCursor();
    }

} // namespace hyprdeck
