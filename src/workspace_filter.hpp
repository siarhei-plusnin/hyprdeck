#pragma once

#include <SharedDefs.hpp>
#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

#include "runtime_types.hpp"

#include <string_view>

namespace hyprdeck {

    class CWorkspaceFilterController {
      public:
        bool             promptOpen() const;
        bool             applied() const;
        std::string_view text() const;
        STextInputState* promptInput();
        void             handleTextChanged(const PHLMONITOR& monitor);

        void openPrompt(const PHLMONITOR& monitor);
        void closePrompt(const PHLMONITOR& monitor);
        void clear(const PHLMONITOR& monitor);
        void resetPromptState();
        void resetState();
        void handleKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
        void render(const PHLMONITOR& monitor) const;

      private:
        void confirmPrompt(const PHLMONITOR& monitor);

        SWorkspaceFilterState m_state;
    };

} // namespace hyprdeck
