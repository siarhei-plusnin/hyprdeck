#include "rendering.hpp"

#include "colors.hpp"
#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "monitor_selector.hpp"
#include "naming.hpp"
#include "plugin.hpp"
#include "shortcuts.hpp"
#include "workspace_filter.hpp"
#include "workspace_preview_renderer.hpp"

#include <output/Monitor.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace hyprdeck {
    namespace {

        bool cardVisible(const std::vector<SWorkspaceCard>& cards, const WORKSPACEID id) {
            return std::ranges::any_of(cards, [&](const auto& card) { return card.id == id; });
        }

    } // namespace

    void COverviewRenderer::renderOverview(const PHLMONITOR& monitor) {
        const auto selectedMonitor = activePlugin()->overview().selectedMonitor();
        if (!selectedMonitor)
            return;

        activePlugin()->layout().recalculateCards(selectedMonitor);

        const auto viewSize     = monitor->m_transformedSize;
        const auto hostSnapshot = activePlugin()->workspacePreviewRenderer().buildSnapshot(monitor);
        if (!activePlugin()->config().activeWorkspaceBackground())
            activePlugin()->workspacePreviewRenderer().renderEmptyWorkspaceBackground(monitor, hostSnapshot);

        std::unordered_map<MONITORID, SWorkspacePreviewSnapshot> snapshots;
        const auto                                               snapshotFor = [&](const PHLMONITOR& sourceMonitor) -> const SWorkspacePreviewSnapshot& {
            if (sourceMonitor->m_id == monitor->m_id)
                return hostSnapshot;

            const auto [entry, inserted] = snapshots.try_emplace(sourceMonitor->m_id);
            if (inserted)
                entry->second = activePlugin()->workspacePreviewRenderer().buildSnapshot(sourceMonitor);
            return entry->second;
        };

        const auto sourceFor = [&](const SWorkspaceCard& card) {
            if (const auto source = activePlugin()->hyprland().monitorFromID(card.sourceMonitorID); source)
                return source;
            return selectedMonitor;
        };

        auto& renderServices = activePlugin()->renderServices();
        auto& animations     = activePlugin()->animations();

        renderServices.pushOpacity(animations.overviewOpacity());
        activePlugin()->renderServices().addRect(CBox{0, 0, viewSize.x, viewSize.y}, colors::overviewScrim());

        for (const auto& card : activePlugin()->layout().cards()) {
            const auto sourceMonitor = sourceFor(card);
            activePlugin()->workspacePreviewRenderer().renderCard(card, sourceMonitor, selectedMonitor, snapshotFor(sourceMonitor));
        }

        for (auto card : activePlugin()->layout().specialCards()) {
            const auto opacity = animations.specialCardOpacity(card.id);
            if (opacity <= 0.001F)
                continue;

            renderServices.pushOpacity(opacity);
            const auto sourceMonitor = sourceFor(card);
            activePlugin()->workspacePreviewRenderer().renderCard(card, sourceMonitor, selectedMonitor, snapshotFor(sourceMonitor));
            renderServices.popOpacity();
        }

        SWorkspaceCard closingCard;
        float          closingOpacity = 1.0F;
        if (animations.closingSpecialCard(closingCard, closingOpacity) && !cardVisible(activePlugin()->layout().specialCards(), closingCard.id)) {
            renderServices.pushOpacity(closingOpacity);
            const auto sourceMonitor = sourceFor(closingCard);
            activePlugin()->workspacePreviewRenderer().renderCard(closingCard, sourceMonitor, selectedMonitor, snapshotFor(sourceMonitor));
            renderServices.popOpacity();
        }

        activePlugin()->monitorSelector().render(monitor);
        activePlugin()->workspaceFilter().render(monitor);
        activePlugin()->shortcuts().renderFooter(monitor);
        activePlugin()->naming().render(monitor);
        activePlugin()->confirmation().render(monitor);
        activePlugin()->shortcuts().renderMenu(monitor);
        renderServices.popOpacity();

        activePlugin()->workspacePreviewRenderer().renderExternalOverlays(monitor, hostSnapshot);
    }

    void COverviewRenderer::renderCursorOverlay(const PHLMONITOR& monitor) {
        auto& renderServices = activePlugin()->renderServices();
        renderServices.updateCursorCache();

        activePlugin()->hyprland().setCursorHidden(false);

        renderServices.updateCursorCache();

        const auto& renderCache = renderServices.cache();
        const auto  pos         = activePlugin()->hyprland().pointerPosition() - monitor->m_position - renderCache.cursorHotspot;

        if (!renderCache.cursorTexture || !renderCache.cursorTexture->ok()) {
            const auto pointer = (activePlugin()->hyprland().pointerPosition() - monitor->m_position) * monitor->m_scale;
            activePlugin()->renderServices().addRect(CBox{pointer.x, pointer.y, 4, 20}, colors::fallbackCursor());
            activePlugin()->renderServices().addRect(CBox{pointer.x, pointer.y, 14, 4}, colors::fallbackCursor());
            return;
        }

        activePlugin()->renderServices().addTexture(renderCache.cursorTexture, CBox{pos * monitor->m_scale, renderCache.cursorSize * monitor->m_scale});
    }

    void COverviewRenderer::clearCache() {
        activePlugin()->renderServices().clearTextTextureCache();
        activePlugin()->renderServices().clearCursorCache();
    }

} // namespace hyprdeck
