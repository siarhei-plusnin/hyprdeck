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
        bool                      isNumericNormalWorkspace(const PHLWORKSPACE& workspace) const;
        bool                      isNamedNormalWorkspace(const PHLWORKSPACE& workspace) const;
        std::string               normalWorkspaceLabel(const PHLWORKSPACE& workspace) const;
        std::vector<PHLWORKSPACE> namedNormalWorkspacesToShow() const;
        PHLMONITOR                workspaceMonitor(const PHLWORKSPACE& workspace) const;
        bool                      workspaceBelongsToMonitor(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const;
        bool                      windowBelongsToMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) const;
        bool                      windowBelongsToWorkspace(const PHLWINDOW& window, const PHLWORKSPACE& workspace) const;
        bool                      workspaceHasAnyWindows(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const;
        bool                      workspaceHasAnyWindows(const PHLWORKSPACE& workspace) const;
        WORKSPACEID               specialWorkspaceId(const PHLWORKSPACE& workspace) const;
        std::string               specialWorkspaceLabel(const PHLWORKSPACE& workspace) const;
        std::vector<PHLWORKSPACE> specialWorkspacesToShow() const;
        bool                      moveSpecialWorkspaceInOrder(WORKSPACEID id, int direction);
        WORKSPACEID               lastWorkspaceToShow() const;
        WORKSPACEID               activeNormalWorkspaceID(const PHLMONITOR& monitor) const;
        WORKSPACEID               activeSpecialWorkspaceID(const PHLMONITOR& monitor) const;
        int                       cardIndexByID(const std::vector<SWorkspaceCard>& cards, WORKSPACEID id) const;
        bool                      cardIsActive(const SWorkspaceCard& card, const PHLMONITOR& monitor) const;
        bool                      cardIsNative(const SWorkspaceCard& card, const PHLMONITOR& monitor) const;
        bool                      cardIsSelected(const SWorkspaceCard& card) const;

      private:
        void                                 reconcileSpecialWorkspaceOrder() const;

        mutable std::vector<PHLWORKSPACEREF> m_specialWorkspaceOrder;
    };

} // namespace hyprdeck
