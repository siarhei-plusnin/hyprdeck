#pragma once

#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>
#include <event/EventBus.hpp>

#include "runtime_types.hpp"

namespace hyprdeck {

    struct SWorkspaceNavigationResult;

    class CNamingController {
      public:
        bool promptOpen() const;
        EPromptMode promptMode() const;
        STextInputState* promptInput();
        void handleTextChanged(const PHLMONITOR& monitor);
        void openNamedSpecialPrompt(const PHLMONITOR& monitor);
        void openRenameSpecialPrompt(const PHLMONITOR& monitor);
        void closePrompt(const PHLMONITOR& monitor);
        void resetPromptState();
        void resetComponent();
        void handleKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
        void render(const PHLMONITOR& monitor) const;

      private:
        bool movePresetSelection(int direction);
        bool useSelectedPreset(const PHLMONITOR& monitor);
        bool applySpecialNavigationResult(const SWorkspaceNavigationResult& result, const PHLMONITOR& monitor, bool centerSpecial);
        void confirmPrompt(const PHLMONITOR& monitor);
        void syncCustomSelectionAfterEdit();

        SNamingState m_state;
    };

} // namespace hyprdeck
