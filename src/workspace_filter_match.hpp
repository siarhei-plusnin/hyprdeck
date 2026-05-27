#pragma once

#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>

#include <vector>

namespace hyprdeck {

    struct SWorkspaceFilterRows {
        std::vector<WORKSPACEID>  normalWorkspaceIDs;
        std::vector<PHLWORKSPACE> specialWorkspaces;
    };

    SWorkspaceFilterRows applyWorkspaceFilter(const PHLMONITOR& monitor, std::vector<WORKSPACEID> normalWorkspaceIDs, std::vector<PHLWORKSPACE> specialWorkspaces);

} // namespace hyprdeck
