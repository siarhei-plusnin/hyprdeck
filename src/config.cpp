#include "config.hpp"

#include "constants.hpp"
#include "env.hpp"
#include "strings.hpp"

#include <algorithm>
#include <optional>
#include <string_view>

namespace hyprdeck {
    namespace {

        const std::vector<std::string>& configuredCommaSeparatedNames(const std::string& rawConfig, CConfigStore::SNameCache& cache,
                                                                       const bool stripSpecialPrefix, const bool normalizeLower, const bool expandEnv) {
            if (rawConfig == cache.raw)
                return cache.names;

            std::vector<std::string> names;
            const auto               expandedConfig = expandEnv ? env::expandVariables(rawConfig) : rawConfig;
            std::string_view         raw = expandedConfig;

            while (!raw.empty()) {
                const auto comma = raw.find(',');

                std::string name = strings::trim(raw.substr(0, comma));
                if (stripSpecialPrefix)
                    name = strings::stripPrefix(name, "special:");

                name = strings::trim(name);
                if (normalizeLower)
                    name = strings::lower(name);

                if (!name.empty() && std::ranges::find(names, name) == names.end())
                    names.push_back(name);

                if (comma == std::string_view::npos)
                    break;

                raw.remove_prefix(comma + 1);
            }

            cache.raw   = rawConfig;
            cache.names = std::move(names);
            return cache.names;
        }

        template <typename T>
        CConfigValue<T>& configValue(std::optional<CConfigValue<T>>& slot, const char* key) {
            if (!slot)
                slot.emplace(key);

            return *slot;
        }

    } // namespace

    std::vector<std::string> CConfigStore::specialWorkspaceNames() {
        auto& value = configValue(m_namedSpecialWorkspaces, "plugin:hyprdeck:named_special_workspaces");
        return configuredCommaSeparatedNames(*value, m_namedSpecialWorkspaceCache, true, false, false);
    }

    double CConfigStore::defaultZoom() {
        auto& value = configValue(m_defaultZoom, "plugin:hyprdeck:default_zoom");
        return std::clamp(static_cast<double>(*value), MIN_ZOOM, MAX_ZOOM);
    }

    bool CConfigStore::activeWorkspaceBackground() {
        auto& value = configValue(m_activeWorkspaceBackground, "plugin:hyprdeck:active_workspace_background");
        return *value;
    }

    std::string CConfigStore::fontFamily() {
        auto& value = configValue(m_fontFamily, "plugin:hyprdeck:font_family");
        return *value;
    }

    EShortcutsFooterMode CConfigStore::shortcutsFooterMode() {
        auto&      value = configValue(m_shortcutsFooterMode, "plugin:hyprdeck:shortcuts_footer");
        const auto mode  = strings::lower(*value);

        if (mode == "hint")
            return EShortcutsFooterMode::HINT;
        if (mode == "none")
            return EShortcutsFooterMode::NONE;

        return EShortcutsFooterMode::FULL;
    }

    const std::vector<std::string>& CConfigStore::blockingOverlayNames() {
        auto& value = configValue(m_blockingOverlays, "plugin:hyprdeck:blocking_overlays");
        return configuredCommaSeparatedNames(*value, m_blockingOverlayCache, false, true, true);
    }

    const std::vector<std::string>& CConfigStore::nonBlockingOverlayNames() {
        auto& value = configValue(m_nonBlockingOverlays, "plugin:hyprdeck:non_blocking_overlays");
        return configuredCommaSeparatedNames(*value, m_nonBlockingOverlayCache, false, true, true);
    }

    const std::vector<std::string>& CConfigStore::displayCaptureOverlayNames() {
        auto& value = configValue(m_displayCaptureOverlays, "plugin:hyprdeck:display_capture_overlays");
        return configuredCommaSeparatedNames(*value, m_displayCaptureOverlayCache, false, true, true);
    }

} // namespace hyprdeck
