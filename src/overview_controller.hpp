#pragma once

#include <desktop/DesktopTypes.hpp>
#include <managers/input/InputManager.hpp>

namespace hyprdeck {

    class COverviewKeyboardController {
      public:
        bool handleKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
    };

} // namespace hyprdeck
