#pragma once

#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

#include "runtime_types.hpp"
#include "shortcut_catalog.hpp"

#include <string>
#include <vector>

namespace hyprdeck {

    class CShortcutMenuController {
      public:
        bool menuOpen() const;
        bool isMenuKey(IKeyboard::SKeyEvent event) const;
        void openMenu(const PHLMONITOR& monitor);
        void closeMenu(const PHLMONITOR& monitor);
        void resetState();
        void handleKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
        void renderFooter(const PHLMONITOR& monitor) const;
        void renderMenu(const PHLMONITOR& monitor);
        STextInputState* searchInput();
        std::vector<SShortcutAction> filteredActions() const;
        std::string                  footerText() const;
        std::string                  searchLabel() const;
        void                         measure(const PHLMONITOR& monitor);

      private:
        SShortcutMenuState m_state;
    };

} // namespace hyprdeck
