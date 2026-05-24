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
    const std::vector<std::string>& configuredBlockingOverlayNames();
    const std::vector<std::string>& configuredNonBlockingOverlayNames();
    const std::vector<std::string>& configuredDisplayCaptureOverlayNames();

} // namespace hyprdeck
