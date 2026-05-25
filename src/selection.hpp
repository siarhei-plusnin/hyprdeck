#pragma once

#include "state.hpp"

#include <helpers/math/Math.hpp>

#include <string>

namespace hyprdeck {

    const SWorkspaceCard* selectedSpecialCard();
    const SWorkspaceCard* selectedNormalCard();
    std::string           selectedSpecialWorkspaceLabel();
    void                  selectWorkspaceAt(const Vector2D& position, const PHLMONITOR& monitor);
    void                  selectRow(ESelectedRow row, const PHLMONITOR& monitor);
    void                  toggleSelection(const PHLMONITOR& monitor);
    void                  openSelection(const PHLMONITOR& monitor);
    void                  switchNormalWorkspaceByID(WORKSPACEID id, const PHLMONITOR& monitor);
    void                  closeSelectedWorkspaceWindows(const PHLMONITOR& monitor);
    void                  moveSelection(int direction, const PHLMONITOR& monitor);
    void                  jumpSelection(int direction, const PHLMONITOR& monitor);

} // namespace hyprdeck
