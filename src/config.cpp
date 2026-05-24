#include "config.hpp"

#include "constants.hpp"

#include <algorithm>
#include <config/ConfigValue.hpp>

#include <cctype>
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

    } // namespace

    std::vector<std::string> configuredSpecialWorkspaceNames() {
        static const auto               PNAMES = CConfigValue<std::string>("plugin:hyprdeck:named_special_workspaces");
        static std::string              cachedRaw;
        static std::vector<std::string> cachedNames;

        const std::string               rawConfig = *PNAMES;
        if (rawConfig == cachedRaw)
            return cachedNames;

        std::vector<std::string> names;
        std::string_view         raw = rawConfig;

        while (!raw.empty()) {
            const auto  comma = raw.find(',');

            std::string name = trim(raw.substr(0, comma));
            if (name.starts_with("special:"))
                name.erase(0, 8);

            name = trim(name);
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

} // namespace hyprdeck
