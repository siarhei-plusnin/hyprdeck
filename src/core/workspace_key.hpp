#pragma once

#include <cstdint>

namespace hyprdeck {

    // Hyprland 0.57 workspace identities are no longer uniformly numeric.
    // Keep a compact runtime key for card layout and selection state.
    using WORKSPACEID = int64_t;

    inline constexpr WORKSPACEID WORKSPACE_INVALID = -1;

} // namespace hyprdeck
