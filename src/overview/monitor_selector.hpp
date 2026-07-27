#pragma once

#include "runtime_types.hpp"

#include <desktop/DesktopTypes.hpp>
#include <helpers/math/Math.hpp>

#include <vector>

namespace hyprdeck {

    struct SMonitorSelectorEntry {
        MONITORID monitorID = MONITOR_INVALID;
        CBox      box;
    };

    class CMonitorSelector {
      public:
        std::vector<SMonitorSelectorEntry> entries(const PHLMONITOR& hostMonitor) const;
        MONITORID                         monitorAt(const Vector2D& position, const PHLMONITOR& hostMonitor) const;
        void                              render(const PHLMONITOR& hostMonitor) const;
    };

} // namespace hyprdeck
