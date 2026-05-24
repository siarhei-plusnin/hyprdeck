#pragma once

namespace hyprdeck {

    inline constexpr double MIN_ZOOM                     = 0.25;
    inline constexpr double DEFAULT_ZOOM                 = 0.85;
    inline constexpr double MAX_ZOOM                     = 1;
    inline constexpr double SPECIAL_CARD_SCALE_THRESHOLD = 0.48;
    inline constexpr double ZOOM_PRESETS[]               = {MIN_ZOOM, 0.35, 0.48, DEFAULT_ZOOM, MAX_ZOOM};

    inline constexpr double NORMAL_CARD_WIDTH_RATIO      = 0.60;
    inline constexpr double NORMAL_CARD_MAX_HEIGHT_RATIO = 0.72;
    inline constexpr double SPECIAL_CARD_HEIGHT_RATIO    = 0.48;

    inline constexpr double CARD_GAP_RATIO               = 0.02;
    inline constexpr double MIN_CARD_GAP                 = 36.0;

    inline constexpr double MIN_SPECIAL_CARD_GAP         = 28.0;
    inline constexpr double SPECIAL_CARD_GAP_SCALE       = 0.72;

    inline constexpr double MIN_ROW_GAP                  = 56.0;
    inline constexpr double ROW_GAP_RATIO                = 0.035;

    inline constexpr double CLICK_DRAG_THRESHOLD         = 8.0;

    inline constexpr double ROW_SCROLL_SCALE             = 16.0;

} // namespace hyprdeck
