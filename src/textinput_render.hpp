#pragma once

#include "textinput.hpp"
#include "render_services.hpp"

#include <helpers/Color.hpp>
#include <helpers/math/Math.hpp>

#include <string_view>

namespace hyprdeck {

    void renderTextInputLine(std::string_view scope, const STextInputState& input, const CBox& box, std::string_view prefix, const CHyprColor& color, int fontSize,
                             int weight, ETextCacheMode cacheMode = ETextCacheMode::NONE);

} // namespace hyprdeck
