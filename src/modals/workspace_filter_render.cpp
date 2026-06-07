#include "workspace_filter.hpp"

#include "colors.hpp"
#include "constants.hpp"
#include "layout.hpp"
#include "plugin.hpp"
#include "runtime_types.hpp"
#include "textinput_render.hpp"

#include <helpers/Monitor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace hyprdeck {
    namespace {

        double filterY(const PHLMONITOR& monitor, const double boxH) {
            const auto& cards        = activePlugin()->layout().cards();
            const auto& specialCards = activePlugin()->layout().specialCards();
            const auto  viewSize     = monitor->m_transformedSize;
            double      y            = (viewSize.y - boxH) / 2.0;

            if (!cards.empty() && !specialCards.empty()) {
                const double normalBottom = cards.front().box.y + cards.front().box.h;
                const double specialTop   = specialCards.front().box.y;
                y                         = normalBottom + ((specialTop - normalBottom - boxH) / 2.0);
            } else if (!cards.empty()) {
                y = cards.front().box.y + cards.front().box.h + (MIN_ROW_GAP / 2.0);
            } else if (!specialCards.empty()) {
                y = specialCards.front().box.y - boxH - (MIN_ROW_GAP / 2.0);
            }

            return std::clamp(y, 24.0, std::max(24.0, viewSize.y - boxH - 24.0));
        }

        SP<Render::ITexture> filterTexture(const std::string& label, const int fontSize, const int weight, const CHyprColor& color = colors::textPrimary()) {
            return activePlugin()->renderServices().textTexture("workspace-filter", label, color, fontSize, weight, ETextCacheMode::NONE);
        }

        CBox snappedBox(const CBox& box, const PHLMONITOR& monitor) {
            const double scale = std::max(1.0, monitor ? monitor->m_scale : 1.0);
            const double x1    = std::round(box.x * scale) / scale;
            const double y1    = std::round(box.y * scale) / scale;
            const double x2    = std::round((box.x + box.w) * scale) / scale;
            const double y2    = std::round((box.y + box.h) * scale) / scale;
            return CBox{x1, y1, x2 - x1, y2 - y1};
        }

        void renderClippedFill(const CBox& box, const CHyprColor& color) {
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(box, 4.0), color, 0, box);
        }

        void renderFilterBox(const CBox& box) {
            renderClippedFill(activePlugin()->renderServices().expandedBox(box, 2.0), colors::componentBorder());
            renderClippedFill(box, colors::componentBackground());
        }

        CBox filterBox(const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const double boxW     = std::min(460.0, viewSize.x - 48.0);
            const double boxH     = 40.0;
            return snappedBox(CBox{(viewSize.x - boxW) / 2.0, filterY(monitor, boxH), boxW, boxH}, monitor);
        }

        void renderFilterPromptBox(const PHLMONITOR& monitor, const STextInputState& input) {
            const auto box = filterBox(monitor);

            renderFilterBox(box);
            renderTextInputLine("workspace-filter", input, box, "Filter: ", colors::textPrimary(), 20, 700);
        }

        void renderAppliedFilterBox(const PHLMONITOR& monitor, const std::string& text) {
            const auto label = filterTexture("Filter: " + text, 20, 700);
            if (!label || !label->ok())
                return;

            const auto   box     = filterBox(monitor);
            const double padding = 14.0;

            renderFilterBox(box);
            const CBox labelClip{box.x + padding, box.y, box.w - (padding * 2.0), box.h};
            activePlugin()->renderServices().addTexture(label, CBox{labelClip.x, box.y + ((box.h - label->m_size.y) / 2.0), label->m_size.x, label->m_size.y}, 1.0F, 0, labelClip);
        }

    } // namespace

    void CWorkspaceFilterController::render(const PHLMONITOR& monitor) const {
        const auto& filter = m_state;
        if (filter.promptOpen) {
            renderFilterPromptBox(monitor, filter.promptInput);
            return;
        }

        if (filter.text.empty())
            return;

        renderAppliedFilterBox(monitor, filter.text);
    }

} // namespace hyprdeck
