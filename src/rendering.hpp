#pragma once

#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    void renderOverview(const PHLMONITOR& monitor);
    void renderCursorOverlay(const PHLMONITOR& monitor);
    void clearRenderCache();

} // namespace hyprdeck
