#pragma once

#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

namespace hyprdeck {

    bool shortcutMenuOpen();
    bool isShortcutMenuKey(IKeyboard::SKeyEvent event);
    void openShortcutMenu(const PHLMONITOR& monitor);
    void closeShortcutMenu(const PHLMONITOR& monitor);
    void resetShortcutState();
    void handleShortcutMenuKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
    void renderShortcutFooter(const PHLMONITOR& monitor);
    void renderShortcutMenu(const PHLMONITOR& monitor);

} // namespace hyprdeck
