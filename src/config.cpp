#include "config.hpp"

#include "constants.hpp"
#include "env.hpp"
#include "strings.hpp"

#include <algorithm>
#include <config/ConfigValue.hpp>

#include <string_view>

namespace hyprdeck {
    namespace {

        const std::vector<std::string>& configuredCommaSeparatedNames(const std::string& rawConfig, std::string& cachedRaw, std::vector<std::string>& cachedNames,
                                                                      const bool stripSpecialPrefix, const bool normalizeLower, const bool expandEnv) {
            if (rawConfig == cachedRaw)
                return cachedNames;

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

            cachedRaw   = rawConfig;
            cachedNames = std::move(names);
            return cachedNames;
        }

    } // namespace

    std::vector<std::string> configuredSpecialWorkspaceNames() {
        static const auto               PNAMES = CConfigValue<std::string>("plugin:hyprdeck:named_special_workspaces");
        static std::string              cachedRaw;
        static std::vector<std::string> cachedNames;

        return configuredCommaSeparatedNames(*PNAMES, cachedRaw, cachedNames, true, false, false);
    }

    double configuredDefaultZoom() {
        static const auto PDEFAULTZOOM = CConfigValue<Config::FLOAT>("plugin:hyprdeck:default_zoom");
        return std::clamp(static_cast<double>(*PDEFAULTZOOM), MIN_ZOOM, MAX_ZOOM);
    }

    bool activeWorkspaceBackground() {
        static const auto PACTIVEBACKGROUND = CConfigValue<Config::BOOL>("plugin:hyprdeck:active_workspace_background");
        return *PACTIVEBACKGROUND;
    }

    std::string configuredFontFamily() {
        static const auto PFONT = CConfigValue<std::string>("plugin:hyprdeck:font_family");
        return *PFONT;
    }

    EShortcutsFooterMode configuredShortcutsFooterMode() {
        static const auto PMODE = CConfigValue<std::string>("plugin:hyprdeck:shortcuts_footer");
        const auto        mode  = strings::lower(*PMODE);

        if (mode == "hint")
            return EShortcutsFooterMode::HINT;
        if (mode == "none")
            return EShortcutsFooterMode::NONE;

        return EShortcutsFooterMode::FULL;
    }

    const std::vector<std::string>& configuredBlockingOverlayNames() {
        static const auto               PNAMES = CConfigValue<std::string>("plugin:hyprdeck:blocking_overlays");
        static std::string              cachedRaw;
        static std::vector<std::string> cachedNames;
        return configuredCommaSeparatedNames(*PNAMES, cachedRaw, cachedNames, false, true, true);
    }

    const std::vector<std::string>& configuredNonBlockingOverlayNames() {
        static const auto               PNAMES = CConfigValue<std::string>("plugin:hyprdeck:non_blocking_overlays");
        static std::string              cachedRaw;
        static std::vector<std::string> cachedNames;
        return configuredCommaSeparatedNames(*PNAMES, cachedRaw, cachedNames, false, true, true);
    }

    const std::vector<std::string>& configuredDisplayCaptureOverlayNames() {
        static const auto               PNAMES = CConfigValue<std::string>("plugin:hyprdeck:display_capture_overlays");
        static std::string              cachedRaw;
        static std::vector<std::string> cachedNames;
        return configuredCommaSeparatedNames(*PNAMES, cachedRaw, cachedNames, false, true, true);
    }

} // namespace hyprdeck
