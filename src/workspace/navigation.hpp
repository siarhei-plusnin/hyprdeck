#pragma once

#include "runtime_types.hpp"

#include <optional>
#include <string>

namespace hyprdeck {

    struct SWorkspaceNavigationResult {
        bool                        success = false;
        std::optional<ESelectedRow> selectedRow;
        std::optional<WORKSPACEID>  selectedNormalID;
        std::optional<WORKSPACEID>  selectedSpecialID;
    };

    class CWorkspaceNavigator {
      public:
        SWorkspaceNavigationResult switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor);
        SWorkspaceNavigationResult hideActiveSpecialWorkspace(const PHLMONITOR& monitor, bool animateIfEmpty = true);
        void                       closeWorkspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor = nullptr);
        SWorkspaceNavigationResult createSimpleSpecialWorkspace(const PHLMONITOR& monitor);
        SWorkspaceNavigationResult createNamedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor);
        SWorkspaceNavigationResult renameSpecialWorkspace(const PHLWORKSPACE& workspace, const std::string& name);
    };

} // namespace hyprdeck
