#include "textinput_render.hpp"

#include "colors.hpp"
#include "plugin.hpp"

#include <helpers/Monitor.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace hyprdeck {
    namespace {

        constexpr double INPUT_TEXT_PADDING = 14.0;
        constexpr double CURSOR_X_OFFSET    = 1.0;
        constexpr int    CURSOR_BLINK_MS    = 500;

        bool             cursorVisible() {
            const auto now = std::chrono::steady_clock::now().time_since_epoch();
            return (std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / CURSOR_BLINK_MS) % 2 == 0;
        }

        double textWidth(std::string_view scope, const std::string& text, const CHyprColor& color, const int fontSize, const int weight, const ETextCacheMode cacheMode) {
            if (text.empty())
                return 0.0;

            const auto texture = activePlugin()->renderServices().textTexture(scope, text, color, fontSize, weight, cacheMode);
            return texture && texture->ok() ? texture->m_size.x : 0.0;
        }

        double cursorWidth(std::string_view scope, const STextInputState& input, const CHyprColor& color, const int fontSize, const int weight, const ETextCacheMode cacheMode) {
            if (input.cursor >= input.text.size())
                return std::max(8.0, static_cast<double>(fontSize) * 0.52);

            const auto glyph = input.text.substr(input.cursor, 1);
            return std::max(8.0, textWidth(scope, glyph, color, fontSize, weight, cacheMode));
        }

        double renderScale() {
            const auto monitor = activePlugin()->overview().monitor();
            return std::max(1.0, monitor ? monitor->m_scale : 1.0);
        }

        CBox pixelCoveredBox(const CBox& box) {
            const double scale = renderScale();
            const double x1    = std::floor(box.x * scale) / scale;
            const double y1    = std::floor(box.y * scale) / scale;
            const double x2    = std::ceil((box.x + box.w) * scale) / scale;
            const double y2    = std::ceil((box.y + box.h) * scale) / scale;
            return CBox{x1, y1, x2 - x1, y2 - y1};
        }

        void renderClippedCursor(const CBox& box) {
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(box, 4.0), colors::textCursor(), 0, box);
        }

        void renderCursor(std::string_view scope, const STextInputState& input, const CBox& box, const std::string& prefix, const CHyprColor& color, const int fontSize,
                          const int weight, const ETextCacheMode cacheMode) {
            if (!cursorVisible())
                return;

            const auto   cursorText = prefix + input.text.substr(0, std::min(input.cursor, input.text.size()));
            const double cursorX    = box.x + INPUT_TEXT_PADDING + textWidth(scope, cursorText, color, fontSize, weight, cacheMode) + CURSOR_X_OFFSET;
            const double cursorH    = std::min(box.h - 10.0, static_cast<double>(fontSize) + 8.0);
            const CBox   cursorBox  = pixelCoveredBox(CBox{cursorX, box.y + ((box.h - cursorH) / 2.0), cursorWidth(scope, input, color, fontSize, weight, cacheMode), cursorH});
            const CBox   clipBox    = pixelCoveredBox(CBox{box.x + INPUT_TEXT_PADDING, box.y, box.w - (INPUT_TEXT_PADDING * 2.0), box.h});
            const auto   clipped    = cursorBox.intersection(clipBox);
            if (!clipped.empty())
                renderClippedCursor(clipped);
        }

    } // namespace

    void renderTextInputLine(const std::string_view scope, const STextInputState& input, const CBox& box, const std::string_view prefix, const CHyprColor& color,
                             const int fontSize, const int weight, const ETextCacheMode cacheMode) {
        const auto text = std::string{prefix} + input.text;

        const CBox clipBox{box.x + INPUT_TEXT_PADDING, box.y, box.w - (INPUT_TEXT_PADDING * 2.0), box.h};
        renderCursor(scope, input, box, std::string{prefix}, color, fontSize, weight, cacheMode);

        const auto texture = activePlugin()->renderServices().textTexture(scope, text, color, fontSize, weight, cacheMode);
        if (texture && texture->ok())
            activePlugin()->renderServices().addTexture(texture, CBox{clipBox.x, box.y + ((box.h - texture->m_size.y) / 2.0), texture->m_size.x, texture->m_size.y}, 1.0F, 0,
                                                        clipBox);
    }

} // namespace hyprdeck
