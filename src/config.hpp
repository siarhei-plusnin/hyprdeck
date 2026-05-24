#pragma once

#include <string>
#include <vector>

namespace hyprdeck {

    enum class EShortcutsFooterMode {
        FULL,
        HINT,
        NONE,
    };

    std::vector<std::string> configuredSpecialWorkspaceNames();
    double                   configuredDefaultZoom();
    bool                     activeWorkspaceBackground();
    std::string              configuredFontFamily();
    EShortcutsFooterMode     configuredShortcutsFooterMode();

} // namespace hyprdeck
