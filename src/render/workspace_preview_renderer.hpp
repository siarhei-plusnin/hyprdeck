#pragma once

#include "runtime_types.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace hyprdeck {

    struct SWorkspacePreviewSnapshot {
        std::array<std::vector<PHLLS>, 4>                       layers;
        std::array<std::vector<PHLLS>, 4>                       externalLayers;
        std::unordered_map<WORKSPACEID, std::vector<PHLWINDOW>> workspaceWindows;
        std::vector<PHLWINDOW>                                  pinnedWindows;
        std::vector<PHLWINDOW>                                  externalWindows;
        PHLWINDOW                                               focusedWindow;
    };

    class CWorkspacePreviewRenderer {
      public:
        SWorkspacePreviewSnapshot buildSnapshot(const PHLMONITOR& monitor) const;
        void renderCard(const SWorkspaceCard& card, const PHLMONITOR& sourceMonitor, const PHLMONITOR& selectedMonitor, const SWorkspacePreviewSnapshot& snapshot) const;
        void renderEmptyWorkspaceBackground(const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) const;
        void renderExternalOverlays(const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) const;
    };

} // namespace hyprdeck
