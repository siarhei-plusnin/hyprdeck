#pragma once

#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>
#include <event/EventBus.hpp>

namespace hyprdeck {

    bool namingPromptOpen();
    void openNamedSpecialPrompt(const PHLMONITOR& monitor);
    void openRenameSpecialPrompt(const PHLMONITOR& monitor);
    void closeNamingPrompt(const PHLMONITOR& monitor);
    void resetNamingPromptState();
    void resetNamingComponent();
    void handleNamingPromptKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);
    void renderNamingPrompt(const PHLMONITOR& monitor);

} // namespace hyprdeck
