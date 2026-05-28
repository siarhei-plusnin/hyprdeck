#pragma once

#include <config/ConfigValue.hpp>

#include <optional>
#include <string>
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
        double                          defaultZoom();
        bool                            activeWorkspaceBackground();
        std::string                     fontFamily();
        EShortcutsFooterMode            shortcutsFooterMode();
        const std::vector<std::string>& blockingOverlayNames();
        const std::vector<std::string>& nonBlockingOverlayNames();
        const std::vector<std::string>& displayCaptureOverlayNames();

        struct SNameCache {
            std::string              raw;
            std::vector<std::string> names;
        };

      private:
        std::optional<CConfigValue<std::string>>   m_namedSpecialWorkspaces;
        std::optional<CConfigValue<Config::FLOAT>> m_defaultZoom;
        std::optional<CConfigValue<Config::BOOL>>  m_activeWorkspaceBackground;
        std::optional<CConfigValue<std::string>>   m_fontFamily;
        std::optional<CConfigValue<std::string>>   m_shortcutsFooterMode;
        std::optional<CConfigValue<std::string>>   m_blockingOverlays;
        std::optional<CConfigValue<std::string>>   m_nonBlockingOverlays;
        std::optional<CConfigValue<std::string>>   m_displayCaptureOverlays;
        SNameCache                   m_namedSpecialWorkspaceCache;
        SNameCache                   m_blockingOverlayCache;
        SNameCache                   m_nonBlockingOverlayCache;
        SNameCache                   m_displayCaptureOverlayCache;
    };

} // namespace hyprdeck
