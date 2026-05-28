#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hyprdeck::strings {

    std::string trim(std::string_view value);
    std::string lower(std::string_view value);
    std::string stripPrefix(std::string_view value, std::string_view prefix);
    std::string normalizeSpecialWorkspaceName(std::string_view value);
    bool        containsLowered(std::string_view value, std::string_view loweredQuery);
    bool        containsInsensitive(std::string_view value, std::string_view query);
    bool        containsAnyLowered(std::string_view value, std::span<const std::string_view> loweredNeedles, const std::vector<std::string>& extraLoweredNeedles);

} // namespace hyprdeck::strings
