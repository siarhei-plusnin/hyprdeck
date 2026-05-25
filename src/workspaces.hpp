#pragma once

#include "state.hpp"

#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>

#include <string>
#include <vector>

namespace hyprdeck {

    bool                      isNormalWorkspace(const PHLWORKSPACE& workspace);
    bool                      windowBelongsToMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor);
    bool                      windowBelongsToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace);
    bool                      workspaceHasAnyWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor);

    WORKSPACEID               specialWorkspaceId(const PHLWORKSPACE& workspace);
    std::string               specialWorkspaceLabel(const PHLWORKSPACE& workspace);
    std::vector<PHLWORKSPACE> specialWorkspacesToShow(const PHLMONITOR& monitor);

    WORKSPACEID               lastWorkspaceToShow(const PHLMONITOR& monitor);
    WORKSPACEID               activeNormalWorkspaceID(const PHLMONITOR& monitor);
    WORKSPACEID               activeSpecialWorkspaceID(const PHLMONITOR& monitor);

    int                       cardIndexByID(const std::vector<SWorkspaceCard>& cards, WORKSPACEID id);
    bool                      cardIsActive(const SWorkspaceCard& card, const PHLMONITOR& monitor);
    bool                      cardIsSelected(const SWorkspaceCard& card);
    void                      ensureSelection(const PHLMONITOR& monitor);

} // namespace hyprdeck
