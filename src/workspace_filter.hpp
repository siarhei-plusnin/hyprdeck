#pragma once

#include <SharedDefs.hpp>
#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

#include <string_view>
#include <vector>

namespace hyprdeck {

    bool                     workspaceFilterPromptOpen();
    bool                     workspaceFilterActive();
    bool                     workspaceFilterApplied();
    std::string_view         workspaceFilterText();
    void                     openWorkspaceFilterPrompt(const PHLMONITOR& monitor);
    void                     closeWorkspaceFilterPrompt(const PHLMONITOR& monitor);
    void                     clearWorkspaceFilter(const PHLMONITOR& monitor);
    void                     resetWorkspaceFilterPromptState();
    void                     resetWorkspaceFilterState();
    void                     handleWorkspaceFilterKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
    void                     renderWorkspaceFilter(const PHLMONITOR& monitor);
    bool                     workspaceMatchesFilter(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor);
    std::vector<WORKSPACEID> filteredNormalWorkspaceIDs(const PHLMONITOR& monitor);

} // namespace hyprdeck
