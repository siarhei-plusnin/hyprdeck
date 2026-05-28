#pragma once

#include "runtime_types.hpp"

#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>

#include <string>
#include <vector>

namespace hyprdeck {

    class CWorkspaceRepository {
      public:
        bool                      isNormalWorkspace(const PHLWORKSPACE& workspace) const;
        bool                      windowBelongsToMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) const;
        bool                      windowBelongsToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace) const;
        bool                      workspaceHasAnyWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const;
        WORKSPACEID               specialWorkspaceId(const PHLWORKSPACE& workspace) const;
        std::string               specialWorkspaceLabel(const PHLWORKSPACE& workspace) const;
        std::vector<PHLWORKSPACE> specialWorkspacesToShow(const PHLMONITOR& monitor) const;
        WORKSPACEID               lastWorkspaceToShow(const PHLMONITOR& monitor) const;
        WORKSPACEID               activeNormalWorkspaceID(const PHLMONITOR& monitor) const;
        WORKSPACEID               activeSpecialWorkspaceID(const PHLMONITOR& monitor) const;
        int                       cardIndexByID(const std::vector<SWorkspaceCard>& cards, WORKSPACEID id) const;
        bool                      cardIsActive(const SWorkspaceCard& card, const PHLMONITOR& monitor) const;
        bool                      cardIsSelected(const SWorkspaceCard& card) const;
    };

} // namespace hyprdeck
