#include "config.hpp"

#include "constants.hpp"
#include "env.hpp"
#include "colors.hpp"
#include "strings.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>

namespace hyprdeck {
    namespace {

        const std::vector<std::string>& configuredCommaSeparatedNames(const std::string& rawConfig, CConfigStore::SNameCache& cache, const bool stripSpecialPrefix,
                                                                      const bool normalizeLower, const bool expandEnv) {
            if (rawConfig == cache.raw)
                return cache.names;

            std::vector<std::string> names;
            const auto               expandedConfig = expandEnv ? env::expandVariables(rawConfig) : rawConfig;
            std::string_view         raw            = expandedConfig;

            while (!raw.empty()) {
                const auto  comma = raw.find(',');

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

    bool CConfigStore::animationsEnabled() {
        auto& value = configValue(m_animationsEnabled, "plugin:hyprdeck:animations");
        return *value;
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

    CHyprColor CConfigStore::outputColor(const std::string_view outputName) {
        auto&             value     = configValue(m_outputColors, "plugin:hyprdeck:output_colors");
        const std::string rawConfig = *value;

        if (rawConfig != m_outputColorCache.raw) {
            std::vector<std::pair<std::string, std::uint32_t>> configuredColors;
            std::string_view                                   raw = rawConfig;

            while (!raw.empty()) {
                const auto    comma = raw.find(',');
                const auto    entry = std::string_view{raw.data(), std::min(comma, raw.size())};
                const auto    colon = entry.find(':');
                const auto    name  = strings::trim(entry.substr(0, colon));
                const auto    hex   = colon == std::string_view::npos ? std::string{} : strings::trim(entry.substr(colon + 1));
                std::uint32_t rgb   = 0;

                if (!name.empty() && hex.size() == 7 && hex.front() == '#') {
                    const auto [end, error] = std::from_chars(hex.data() + 1, hex.data() + hex.size(), rgb, 16);
                    if (error == std::errc{} && end == hex.data() + hex.size()) {
                        const auto existing = std::ranges::find_if(configuredColors, [&name](const auto& configured) { return configured.first == name; });
                        if (existing == configuredColors.end())
                            configuredColors.emplace_back(name, rgb);
                        else
                            existing->second = rgb;
                    }
                }

                if (comma == std::string_view::npos)
                    break;

                raw.remove_prefix(comma + 1);
            }

            m_outputColorCache.raw    = rawConfig;
            m_outputColorCache.colors = std::move(configuredColors);
        }

        const auto configured = std::ranges::find_if(m_outputColorCache.colors, [outputName](const auto& entry) { return entry.first == outputName; });
        return configured == m_outputColorCache.colors.end() ? colors::automaticOutputColor(outputName) : colors::rgb(configured->second);
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
