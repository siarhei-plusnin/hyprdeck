#include "overview_interaction.hpp"

#include "constants.hpp"
#include "layout.hpp"
#include "monitors.hpp"
#include "overview.hpp"
#include "rendering.hpp"
#include "selection.hpp"
#include "state.hpp"

#include <helpers/Monitor.hpp>
#include <managers/PointerManager.hpp>
#include <render/Renderer.hpp>

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

namespace hyprdeck {

    void handleOverviewMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        auto& current = state();
        if (!current.session.active)
            return;

        info.cancelled = true;

        const auto monitor = overviewMonitor();
        if (!monitor) {
            closeOverview();
            return;
        }

        const auto renderPos = (position - monitor->m_position) * monitor->m_scale;

        g_pPointerManager->damageCursor(monitor);

        if (!current.interaction.dragging)
            return;

        if (current.interaction.dragRow == EDragRow::SPECIAL)
            current.layout.specialCameraX = clampSpecialCamera(current.interaction.dragStartSpecialCameraX - (renderPos.x - current.interaction.dragStart.x), current.layout.specialCards.size());
        else
            current.layout.cameraX = clampCamera(current.interaction.dragStartCameraX - (renderPos.x - current.interaction.dragStart.x));

        invalidateLayout();

        g_pHyprRenderer->damageMonitor(monitor);
    }

    void handleOverviewMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        auto& current = state();
        if (!current.session.active)
            return;

        if (event.button == BTN_RIGHT) {
            closeOverview();
            return;
        }

        if (event.button != BTN_LEFT)
            return;

        info.cancelled = true;

        const auto monitor = overviewMonitor();
        if (!monitor) {
            closeOverview();
            return;
        }

        recalculateCards(monitor);

        const auto pos = cursorRenderPos(monitor);

        if (event.state == WL_POINTER_BUTTON_STATE_PRESSED) {
            auto& interaction                   = current.interaction;
            interaction.dragging                = true;
            interaction.dragRow                 = dragRowAt(pos);
            interaction.dragStart               = pos;
            interaction.dragStartCameraX        = current.layout.cameraX;
            interaction.dragStartSpecialCameraX = current.layout.specialCameraX;
            return;
        }

        if (event.state == WL_POINTER_BUTTON_STATE_RELEASED && current.interaction.dragging) {
            current.interaction.dragging = false;
            current.interaction.dragRow  = EDragRow::NONE;

            if (current.interaction.dragStart.distance(pos) < CLICK_DRAG_THRESHOLD)
                selectWorkspaceAt(pos, monitor);

            g_pHyprRenderer->damageMonitor(monitor);
        }
    }

    void handleOverviewMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        if (!state().session.active)
            return;

        info.cancelled = true;

        const auto monitor = overviewMonitor();
        if (!monitor) {
            closeOverview();
            return;
        }

        recalculateCards(monitor);

        auto&      current = state();
        const auto pos     = cursorRenderPos(monitor);
        if (dragRowAt(pos) == EDragRow::SPECIAL)
            current.layout.specialCameraX = clampSpecialCamera(current.layout.specialCameraX + (event.delta * ROW_SCROLL_SCALE), current.layout.specialCards.size());
        else
            current.layout.cameraX = clampCamera(current.layout.cameraX + (event.delta * ROW_SCROLL_SCALE));

        invalidateLayout();

        g_pHyprRenderer->damageMonitor(monitor);
    }

    void handleOverviewRenderStage(eRenderStage stage) {
        const auto& current = state();
        if (!current.session.active || stage != RENDER_LAST_MOMENT)
            return;

        const auto renderMonitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
        if (!renderMonitor || renderMonitor->m_id != current.session.monitorID)
            return;

        renderOverview(renderMonitor);
        renderCursorOverlay(renderMonitor);
    }

} // namespace hyprdeck
