#pragma once

#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    class COverviewRenderer {
      public:
        void renderOverview(const PHLMONITOR& monitor);
        void renderCursorOverlay(const PHLMONITOR& monitor);
        void clearCache();
    };

} // namespace hyprdeck
