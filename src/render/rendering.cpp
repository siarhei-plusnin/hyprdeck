#include "rendering.hpp"

#include "colors.hpp"
#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "plugin.hpp"
#include "shortcuts.hpp"
#include "workspace_filter.hpp"
#include "workspace_preview_renderer.hpp"

#include <helpers/Monitor.hpp>

namespace hyprdeck {

    void COverviewRenderer::renderOverview(const PHLMONITOR& monitor) {
        activePlugin()->layout().recalculateCards(monitor);

        const auto viewSize = monitor->m_transformedSize;
        const auto snapshot = activePlugin()->workspacePreviewRenderer().buildSnapshot(monitor);
        if (!activePlugin()->config().activeWorkspaceBackground())
            activePlugin()->workspacePreviewRenderer().renderEmptyWorkspaceBackground(monitor, snapshot);

        auto& renderServices = activePlugin()->renderServices();
        auto& animations     = activePlugin()->animations();

        renderServices.pushOpacity(animations.overviewOpacity());
        activePlugin()->renderServices().addRect(CBox{0, 0, viewSize.x, viewSize.y}, colors::overviewScrim());

        for (const auto& card : activePlugin()->layout().cards())
            activePlugin()->workspacePreviewRenderer().renderCard(card, monitor, snapshot);

        for (auto card : activePlugin()->layout().specialCards()) {
            const auto opacity = animations.specialCardOpacity(card.id);
            if (opacity <= 0.001F)
                continue;

            renderServices.pushOpacity(opacity);
            activePlugin()->workspacePreviewRenderer().renderCard(card, monitor, snapshot);
            renderServices.popOpacity();
        }

        activePlugin()->workspaceFilter().render(monitor);
        activePlugin()->shortcuts().renderFooter(monitor);
        activePlugin()->naming().render(monitor);
        activePlugin()->confirmation().render(monitor);
        activePlugin()->shortcuts().renderMenu(monitor);
        renderServices.popOpacity();

        activePlugin()->workspacePreviewRenderer().renderExternalOverlays(monitor, snapshot);
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
