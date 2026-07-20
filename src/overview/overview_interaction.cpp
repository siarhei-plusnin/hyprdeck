#include "overview_interaction.hpp"

#include "constants.hpp"
#include "layout.hpp"
#include "overview.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "selection.hpp"
#include "runtime_types.hpp"

#include <output/Monitor.hpp>
#include <render/Renderer.hpp>

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

namespace hyprdeck {

    void COverviewPointerController::handleMouseMove(Vector2D position, Event::SCallbackInfo& info, const PHLMONITOR& monitor) {
        auto& interaction = m_state;
        if (!interaction.dragging && activePlugin()->overlays().pointerOverNotificationOverlay(monitor))
            return;

        info.cancelled = true;

        const auto renderPos = (position - monitor->m_position) * monitor->m_scale;

        activePlugin()->hyprland().damageCursor(monitor);

        if (!interaction.dragging)
            return;

        if (interaction.dragRow == EDragRow::SPECIAL)
            activePlugin()->layout().setSpecialCameraX(activePlugin()->layout().clampSpecialCamera(interaction.dragStartSpecialCameraX - (renderPos.x - interaction.dragStart.x),
                                                                                                   activePlugin()->layout().specialCardCount()));
        else
            activePlugin()->layout().setCameraX(activePlugin()->layout().clampCamera(interaction.dragStartCameraX - (renderPos.x - interaction.dragStart.x)));

        activePlugin()->layout().invalidate();

        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void COverviewPointerController::handleMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor) {
        auto& interaction = m_state;
        if (!interaction.dragging && activePlugin()->overlays().pointerOverNotificationOverlay(monitor))
            return;

        if (event.button == BTN_RIGHT) {
            activePlugin()->overview().close();
            return;
        }

        if (event.button != BTN_LEFT)
            return;

        info.cancelled = true;

        activePlugin()->layout().recalculateCards(monitor);

        const auto pos = activePlugin()->layout().cursorRenderPos(monitor);

        if (event.state == WL_POINTER_BUTTON_STATE_PRESSED) {
            activePlugin()->animations().cancelCameraAnimations();
            interaction.dragging                = true;
            interaction.dragRow                 = activePlugin()->layout().dragRowAt(pos);
            interaction.dragStart               = pos;
            interaction.dragStartCameraX        = activePlugin()->layout().cameraX();
            interaction.dragStartSpecialCameraX = activePlugin()->layout().specialCameraX();
            return;
        }

        if (event.state == WL_POINTER_BUTTON_STATE_RELEASED && interaction.dragging) {
            interaction.dragging = false;
            interaction.dragRow  = EDragRow::NONE;

            if (interaction.dragStart.distance(pos) < CLICK_DRAG_THRESHOLD)
                activePlugin()->selection().selectWorkspaceAt(pos, monitor);

            activePlugin()->hyprland().damageMonitor(monitor);
        }
    }

    void COverviewPointerController::handleMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor) {
        if (activePlugin()->overlays().pointerOverNotificationOverlay(monitor))
            return;

        info.cancelled = true;

        activePlugin()->layout().recalculateCards(monitor);

        const auto pos = activePlugin()->layout().cursorRenderPos(monitor);
        if (activePlugin()->layout().dragRowAt(pos) == EDragRow::SPECIAL) {
            activePlugin()->animations().cancelSpecialCameraAnimation();
            activePlugin()->layout().setSpecialCameraX(activePlugin()->layout().clampSpecialCamera(activePlugin()->layout().specialCameraX() + (event.delta * ROW_SCROLL_SCALE),
                                                                                                   activePlugin()->layout().specialCardCount()));
        } else {
            activePlugin()->animations().cancelNormalCameraAnimation();
            activePlugin()->layout().setCameraX(activePlugin()->layout().clampCamera(activePlugin()->layout().cameraX() + (event.delta * ROW_SCROLL_SCALE)));
        }

        activePlugin()->layout().invalidate();

        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void COverviewPointerController::resetState() {
        m_state.dragging = false;
        m_state.dragRow  = EDragRow::NONE;
    }

} // namespace hyprdeck
