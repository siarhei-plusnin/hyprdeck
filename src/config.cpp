#include "config.hpp"

#include "constants.hpp"

#include <algorithm>
#include <config/ConfigValue.hpp>

#include <cctype>
#include <cstdlib>
#include <string_view>

namespace hyprdeck {
    namespace {

        std::string trim(std::string_view value) {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
                value.remove_prefix(1);

            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.remove_suffix(1);

            return std::string{value};
        }

        std::string lower(std::string_view value) {
            std::string lowered;
            lowered.reserve(value.size());
            for (const char character : value)
                lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));

            return lowered;
        }

        bool envNameCharacter(const char character) {
            return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
        }

        std::string envValue(std::string_view name) {
            if (name.empty())
                return "";

            const auto variable = std::string{name};
            if (const auto* value = std::getenv(variable.c_str()))
                return value;

            return "";
        }

        std::string expandEnvVariables(std::string_view value) {
            std::string expanded;
            expanded.reserve(value.size());

            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] != '$') {
                    expanded.push_back(value[i]);
                    continue;
                }

                if (i + 1 >= value.size()) {
                    expanded.push_back(value[i]);
                    continue;
                }

                if (value[i + 1] == '{') {
                    const auto end = value.find('}', i + 2);
                    if (end == std::string_view::npos) {
                        expanded.push_back(value[i]);
                        continue;
                    }

                    expanded += envValue(value.substr(i + 2, end - i - 2));
                    i = end;
                    continue;
                }

                if (!envNameCharacter(value[i + 1])) {
                    expanded.push_back(value[i]);
                    continue;
                }

                size_t end = i + 1;
                while (end < value.size() && envNameCharacter(value[end]))
                    ++end;

                expanded += envValue(value.substr(i + 1, end - i - 1));
                i = end - 1;
            }

            return expanded;
        }

        const std::vector<std::string>& configuredCommaSeparatedNames(const std::string& rawConfig, std::string& cachedRaw, std::vector<std::string>& cachedNames,
                                                                      const bool stripSpecialPrefix, const bool normalizeLower, const bool expandEnv) {
            if (rawConfig == cachedRaw)
                return cachedNames;

            std::vector<std::string> names;
            const auto               expandedConfig = expandEnv ? expandEnvVariables(rawConfig) : rawConfig;
            std::string_view         raw = expandedConfig;

            while (!raw.empty()) {
                const auto comma = raw.find(',');

                std::string name = trim(raw.substr(0, comma));
                if (stripSpecialPrefix && name.starts_with("special:"))
                    name.erase(0, 8);

                name = trim(name);
                if (normalizeLower)
                    name = lower(name);

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
        const auto        mode  = lower(*PMODE);

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
