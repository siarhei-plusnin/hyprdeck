#include "workspace_filter_render.hpp"

#include "colors.hpp"
#include "constants.hpp"
#include "layout.hpp"
#include "state.hpp"
#include "textinput_render.hpp"
#include "ui.hpp"

#include <helpers/Monitor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <string>

namespace hyprdeck {
    namespace {

        double filterY(const PHLMONITOR& monitor, const double boxH) {
            const auto& layout   = state().layout;
            const auto  viewSize = monitor->m_transformedSize;
            double      y        = (viewSize.y - boxH) / 2.0;

            if (!layout.cards.empty() && !layout.specialCards.empty()) {
                const double normalBottom = layout.cards.front().box.y + layout.cards.front().box.h;
                const double specialTop   = layout.specialCards.front().box.y;
                y                         = normalBottom + ((specialTop - normalBottom - boxH) / 2.0);
            } else if (!layout.cards.empty()) {
                y = layout.cards.front().box.y + layout.cards.front().box.h + (MIN_ROW_GAP / 2.0);
            } else if (!layout.specialCards.empty()) {
                y = layout.specialCards.front().box.y - boxH - (MIN_ROW_GAP / 2.0);
            }

            return std::clamp(y, 24.0, std::max(24.0, viewSize.y - boxH - 24.0));
        }

        SP<Render::ITexture> filterTexture(const std::string& label, const int fontSize, const int weight, const CHyprColor& color = colors::textPrimary()) {
            return textTexture("workspace-filter", label, color, fontSize, weight, ETextCacheMode::NONE);
        }

        void renderFilterBox(const CBox& box) {
            addRect(expanded(box, 2.0), colors::componentBorder());
            addRect(box, colors::componentBackground());
        }

        CBox filterBox(const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const double boxW     = std::min(460.0, viewSize.x - 48.0);
            const double boxH     = 40.0;
            return CBox{(viewSize.x - boxW) / 2.0, filterY(monitor, boxH), boxW, boxH};
        }

        void renderFilterPromptBox(const PHLMONITOR& monitor) {
            const auto box = filterBox(monitor);

            renderFilterBox(box);
            renderTextInputLine("workspace-filter", state().filter.promptInput, box, "Filter: ", colors::textPrimary(), 20, 700);
        }

        void renderAppliedFilterBox(const PHLMONITOR& monitor, const std::string& text) {
            const auto label = filterTexture("Filter: " + text, 20, 700);
            if (!label || !label->ok())
                return;

            const auto   box     = filterBox(monitor);
            const double padding = 14.0;

            renderFilterBox(box);
            const CBox labelClip{box.x + padding, box.y, box.w - (padding * 2.0), box.h};
            addTexture(label, CBox{labelClip.x, box.y + ((box.h - label->m_size.y) / 2.0), label->m_size.x, label->m_size.y}, 1.0F, 0, labelClip);
        }

    } // namespace

    void renderWorkspaceFilter(const PHLMONITOR& monitor) {
        const auto& filter = state().filter;
        if (filter.promptOpen) {
            renderFilterPromptBox(monitor);
            return;
        }

        if (filter.text.empty())
            return;

        renderAppliedFilterBox(monitor, filter.text);
    }

} // namespace hyprdeck
