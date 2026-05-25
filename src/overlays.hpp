#pragma once

#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    bool layerIsExternalOverlay(const PHLLS& layer);
    bool layerShouldRenderOverOverview(const PHLLS& layer);
    bool windowIsExternalOverlay(const PHLWINDOW& window);
    bool externalOverlayActive(const PHLMONITOR& monitor);
    bool pointerOverNotificationOverlay(const PHLMONITOR& monitor);

} // namespace hyprdeck
