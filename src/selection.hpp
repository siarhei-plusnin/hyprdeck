#pragma once

#include "runtime_types.hpp"

#include <helpers/math/Math.hpp>

#include <string>

namespace hyprdeck {

    class CSelectionController {
      public:
        SSelectionState snapshot() const;
        ESelectedRow    selectedRow() const;
        WORKSPACEID     selectedNormalID() const;
        WORKSPACEID     selectedSpecialID() const;
        WORKSPACEID     lastActiveNormalID() const;
        WORKSPACEID     lastActiveSpecialID() const;
        void            resetState();
        void            setSelectedRow(ESelectedRow row);
        void            setSelectedNormalID(WORKSPACEID id);
        void            setSelectedSpecialID(WORKSPACEID id);
        void            setLastActiveNormalID(WORKSPACEID id);
        void            setLastActiveSpecialID(WORKSPACEID id);
        void            setActiveSelection(WORKSPACEID activeNormalID, WORKSPACEID activeSpecialID);
        void            ensureSelection(const PHLMONITOR& monitor);

        const SWorkspaceCard* selectedSpecialCard() const;
        const SWorkspaceCard* selectedNormalCard() const;
        std::string           selectedSpecialWorkspaceLabel() const;
        void                  selectWorkspaceAt(const Vector2D& position, const PHLMONITOR& monitor);
        void                  selectRow(ESelectedRow row, const PHLMONITOR& monitor);
        void                  toggleSelection(const PHLMONITOR& monitor);
        void                  openSelection(const PHLMONITOR& monitor);
        void                  switchNormalWorkspaceByID(WORKSPACEID id, const PHLMONITOR& monitor);
        void                  closeSelectedWorkspaceWindows(const PHLMONITOR& monitor);
        void                  moveSelection(int direction, const PHLMONITOR& monitor);
        void                  jumpSelection(int direction, const PHLMONITOR& monitor);

      private:
        SSelectionState m_state;
    };

} // namespace hyprdeck
