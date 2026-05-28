#include "strings.hpp"

#include <cctype>

namespace hyprdeck::strings {

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

    std::string stripPrefix(std::string_view value, std::string_view prefix) {
        if (value.starts_with(prefix))
            value.remove_prefix(prefix.size());

        return std::string{value};
    }

    std::string normalizeSpecialWorkspaceName(std::string_view value) {
        return trim(stripPrefix(trim(value), "special:"));
    }

    bool containsLowered(std::string_view value, std::string_view loweredQuery) {
        return lower(value).contains(loweredQuery);
    }

    bool containsInsensitive(std::string_view value, std::string_view query) {
        return containsLowered(value, lower(query));
    }

    bool containsAnyLowered(std::string_view value, std::span<const std::string_view> loweredNeedles, const std::vector<std::string>& extraLoweredNeedles) {
        const auto lowered = lower(value);

        for (const auto needle : loweredNeedles) {
            if (lowered.contains(needle))
                return true;
        }

        for (const auto& needle : extraLoweredNeedles) {
            if (lowered.contains(needle))
                return true;
        }

        return false;
    }

} // namespace hyprdeck::strings
