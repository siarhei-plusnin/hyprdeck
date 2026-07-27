#include "monitor_selector.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "overview.hpp"
#include "plugin.hpp"
#include "render_services.hpp"

#include <output/Monitor.hpp>

namespace hyprdeck {
    namespace {

        constexpr double     SELECTOR_MARGIN_X  = 24.0;
        constexpr double     SELECTOR_MARGIN_Y  = 24.0;
        constexpr double     SELECTOR_PADDING_X = 14.0;
        constexpr double     SELECTOR_PADDING_Y = 9.0;
        constexpr double     SELECTOR_GAP       = 8.0;

        SP<Render::ITexture> outputNameTexture(const std::string& name) {
            return activePlugin()->renderServices().textTexture("monitor-selector", name, colors::textPrimary(), 18, 700);
        }

    } // namespace

    std::vector<SMonitorSelectorEntry> CMonitorSelector::entries(const PHLMONITOR& hostMonitor) const {
        std::vector<SMonitorSelectorEntry> result;
        if (!hostMonitor)
            return result;

        double x = SELECTOR_MARGIN_X;
        for (const auto& monitor : activePlugin()->hyprland().monitors()) {
            if (!monitor)
                continue;

            const auto texture = outputNameTexture(monitor->m_name);
            if (!texture || !texture->ok())
                continue;

            const CBox box{x, SELECTOR_MARGIN_Y, texture->m_size.x + (SELECTOR_PADDING_X * 2.0), texture->m_size.y + (SELECTOR_PADDING_Y * 2.0)};
            result.push_back(SMonitorSelectorEntry{.monitorID = monitor->m_id, .box = box});
            x += box.w + SELECTOR_GAP;
        }

        return result;
    }

    MONITORID CMonitorSelector::monitorAt(const Vector2D& position, const PHLMONITOR& hostMonitor) const {
        for (const auto& entry : entries(hostMonitor)) {
            if (entry.box.containsPoint(position))
                return entry.monitorID;
        }

        return MONITOR_INVALID;
    }

    void CMonitorSelector::render(const PHLMONITOR& hostMonitor) const {
        for (const auto& entry : entries(hostMonitor)) {
            const auto monitor = activePlugin()->hyprland().monitorFromID(entry.monitorID);
            if (!monitor)
                continue;

            const bool selected = entry.monitorID == activePlugin()->overview().selectedMonitorID();
            const auto border   = activePlugin()->config().outputColor(monitor->m_name);
            const auto texture  = outputNameTexture(monitor->m_name);
            if (!texture || !texture->ok())
                continue;

            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(entry.box, 2.0), border);
            activePlugin()->renderServices().addRect(entry.box, selected ? colors::componentSelected() : colors::componentBackground());
            activePlugin()->renderServices().addTexture(texture, CBox{entry.box.x + SELECTOR_PADDING_X, entry.box.y + SELECTOR_PADDING_Y, texture->m_size.x, texture->m_size.y});
        }
    }

} // namespace hyprdeck
