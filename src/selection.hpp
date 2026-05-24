#pragma once

#include "state.hpp"

#include <helpers/math/Math.hpp>

#include <string>

namespace hyprdeck {

    const SWorkspaceCard* selectedSpecialCard();
    const SWorkspaceCard* selectedNormalCard();
    std::string           selectedSpecialWorkspaceLabel();
    void                  selectWorkspaceAt(const Vector2D& position, const PHLMONITOR& monitor);
    void                  selectKeyboardRow(ESelectedRow row, const PHLMONITOR& monitor);
    void                  spaceKeyboardSelection(const PHLMONITOR& monitor);
    void                  openSelectedSpecialAndClose(const PHLMONITOR& monitor);
    void                  switchNormalWorkspaceByID(WORKSPACEID id, const PHLMONITOR& monitor);
    void                  closeSelectedWorkspaceWindows(const PHLMONITOR& monitor);
    void                  moveKeyboardSelection(int direction, const PHLMONITOR& monitor);
    void                  jumpKeyboardSelection(int direction, const PHLMONITOR& monitor);

} // namespace hyprdeck
