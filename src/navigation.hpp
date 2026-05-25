#pragma once

#include "state.hpp"

#include <string>

namespace hyprdeck {

    bool switchWorkspaceCard(const SWorkspaceCard& card, const PHLMONITOR& monitor);
    bool cleanupPendingSpecialWorkspace(const PHLMONITOR& monitor);
    void closeWorkspaceWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor = nullptr);
    void createSimpleSpecialWorkspace(const PHLMONITOR& monitor);
    bool createNamedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor);
    bool renameSelectedSpecialWorkspace(const std::string& name, const PHLMONITOR& monitor);

} // namespace hyprdeck
