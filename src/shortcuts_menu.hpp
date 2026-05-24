#pragma once

#include "shortcut_catalog.hpp"

#include <desktop/DesktopTypes.hpp>

#include <string>
#include <vector>

namespace hyprdeck {

    std::vector<SShortcutAction> filteredShortcutMenuActions();
    std::string                  shortcutFooterText();
    std::string                  shortcutSearchLabel();
    void                         measureShortcutMenu(const PHLMONITOR& monitor);

} // namespace hyprdeck
