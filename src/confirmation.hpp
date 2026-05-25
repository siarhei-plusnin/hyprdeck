#pragma once

#include <SharedDefs.hpp>
#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    bool confirmationPromptOpen();
    void openCloseNormalWorkspaceConfirmation(WORKSPACEID workspaceID, const PHLMONITOR& monitor);
    void closeConfirmationPrompt(const PHLMONITOR& monitor);
    void resetConfirmationState();
    void handleConfirmationKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
    void renderConfirmationPrompt(const PHLMONITOR& monitor);

} // namespace hyprdeck
