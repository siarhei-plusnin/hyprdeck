#pragma once

#include <string>
#include <string_view>

namespace hyprdeck::env {

    std::string expandVariables(std::string_view value);

} // namespace hyprdeck::env
