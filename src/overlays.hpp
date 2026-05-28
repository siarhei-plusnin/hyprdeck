#pragma once

#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    class COverlayPolicy {
      public:
        bool layerIsExternalOverlay(const PHLLS& layer) const;
        bool layerShouldRenderOverOverview(const PHLLS& layer) const;
        bool windowIsExternalOverlay(const PHLWINDOW& window) const;
        bool externalOverlayActive(const PHLMONITOR& monitor) const;
        bool pointerOverNotificationOverlay(const PHLMONITOR& monitor) const;
    };

} // namespace hyprdeck
