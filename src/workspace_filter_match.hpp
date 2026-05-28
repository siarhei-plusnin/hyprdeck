#pragma once

#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>

#include <vector>

namespace hyprdeck {

    struct SWorkspaceFilterRows {
        std::vector<WORKSPACEID>  normalWorkspaceIDs;
        std::vector<PHLWORKSPACE> specialWorkspaces;
    };

    class CWorkspaceFilterMatcher {
      public:
        SWorkspaceFilterRows apply(const PHLMONITOR& monitor, std::vector<WORKSPACEID> normalWorkspaceIDs, std::vector<PHLWORKSPACE> specialWorkspaces) const;
    };

} // namespace hyprdeck
