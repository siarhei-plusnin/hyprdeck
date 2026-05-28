#pragma once

#include <SharedDefs.hpp>
#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

#include "runtime_types.hpp"

namespace hyprdeck {

    class CConfirmationController {
      public:
        bool promptOpen() const;
        void openCloseNormalWorkspaceConfirmation(WORKSPACEID workspaceID, const PHLMONITOR& monitor);
        void closePrompt(const PHLMONITOR& monitor);
        void resetState();
        void handleKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
        void render(const PHLMONITOR& monitor);

      private:
        void confirmCloseNormalWorkspace(const PHLMONITOR& monitor);

        SConfirmationState m_state;
    };

} // namespace hyprdeck
