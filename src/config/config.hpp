#pragma once

#include <config/ConfigValue.hpp>
#include <helpers/Color.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hyprdeck {

    enum class EShortcutsFooterMode {
        FULL,
        HINT,
        NONE,
    };

    class CConfigStore {
      public:
        std::vector<std::string>        specialWorkspaceNames();
        Config::INTEGER                 minimumNumberedWorkspaces();
        Config::INTEGER                 numberedWorkspacesAfterLast();
        double                          defaultZoom();
        bool                            animationsEnabled();
        bool                            activeWorkspaceBackground();
        std::string                     fontFamily();
        EShortcutsFooterMode            shortcutsFooterMode();
        CHyprColor                      outputColor(std::string_view outputName);
        const std::vector<std::string>& blockingOverlayNames();
        const std::vector<std::string>& nonBlockingOverlayNames();
        const std::vector<std::string>& displayCaptureOverlayNames();

        struct SNameCache {
            std::string              raw;
            std::vector<std::string> names;
        };

      private:
        struct SOutputColorCache {
            std::string                                        raw;
            std::vector<std::pair<std::string, std::uint32_t>> colors;
        };

        std::optional<CConfigValue<std::string>>     m_namedSpecialWorkspaces;
        std::optional<CConfigValue<Config::INTEGER>> m_minimumNumberedWorkspaces;
        std::optional<CConfigValue<Config::INTEGER>> m_numberedWorkspacesAfterLast;
        std::optional<CConfigValue<Config::FLOAT>>   m_defaultZoom;
        std::optional<CConfigValue<Config::BOOL>>    m_animationsEnabled;
        std::optional<CConfigValue<Config::BOOL>>    m_activeWorkspaceBackground;
        std::optional<CConfigValue<std::string>>     m_fontFamily;
        std::optional<CConfigValue<std::string>>     m_shortcutsFooterMode;
        std::optional<CConfigValue<std::string>>     m_outputColors;
        std::optional<CConfigValue<std::string>>     m_blockingOverlays;
        std::optional<CConfigValue<std::string>>     m_nonBlockingOverlays;
        std::optional<CConfigValue<std::string>>     m_displayCaptureOverlays;
        SNameCache                                   m_namedSpecialWorkspaceCache;
        SOutputColorCache                            m_outputColorCache;
        SNameCache                                   m_blockingOverlayCache;
        SNameCache                                   m_nonBlockingOverlayCache;
        SNameCache                                   m_displayCaptureOverlayCache;
    };

} // namespace hyprdeck
