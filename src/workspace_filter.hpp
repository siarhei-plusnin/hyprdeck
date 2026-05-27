#pragma once

#include <SharedDefs.hpp>
#include <devices/IKeyboard.hpp>
#include <desktop/DesktopTypes.hpp>

#include <string_view>

namespace hyprdeck {

    bool             workspaceFilterPromptOpen();
    bool             workspaceFilterApplied();
    std::string_view workspaceFilterText();
    void             openWorkspaceFilterPrompt(const PHLMONITOR& monitor);
    void             closeWorkspaceFilterPrompt(const PHLMONITOR& monitor);
    void             clearWorkspaceFilter(const PHLMONITOR& monitor);
    void             resetWorkspaceFilterPromptState();
    void             resetWorkspaceFilterState();
    void             handleWorkspaceFilterKey(IKeyboard::SKeyEvent event, const PHLMONITOR& monitor);

} // namespace hyprdeck
