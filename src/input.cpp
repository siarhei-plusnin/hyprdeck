#include "input.hpp"

#include "confirmation.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "overview_controller.hpp"
#include "overview_interaction.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "shortcuts.hpp"
#include "runtime_types.hpp"
#include "workspace_filter.hpp"

namespace hyprdeck {
    PHLMONITOR CInputRouter::activeInputMonitor() const {
        if (!activePlugin()->overview().active())
            return nullptr;

        const auto monitor = activePlugin()->overview().monitor();
        if (!monitor)
            activePlugin()->overview().close();

        return monitor;
    }

    bool CInputRouter::inputBlockedByExternalOverlay(const PHLMONITOR& monitor) const {
        return activePlugin()->overlays().externalOverlayActive(monitor);
    }

    EInputMode CInputRouter::activeInputMode() const {
        if (!activePlugin()->overview().active())
            return EInputMode::INACTIVE;

        if (activePlugin()->shortcuts().menuOpen())
            return EInputMode::SHORTCUTS;

        if (activePlugin()->naming().promptOpen())
            return EInputMode::NAMING;

        if (activePlugin()->workspaceFilter().promptOpen())
            return EInputMode::FILTER;

        if (activePlugin()->confirmation().promptOpen())
            return EInputMode::CONFIRMATION;

        return EInputMode::OVERVIEW;
    }

    void CInputRouter::onMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseMove(position, info, monitor);
    }

    void CInputRouter::onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseButton(event, info, monitor);
    }

    void CInputRouter::onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        activePlugin()->overviewPointer().handleMouseAxis(event, info, monitor);
    }

    void CInputRouter::onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        const auto mode = activeInputMode();
        if (mode == EInputMode::INACTIVE)
            return;

        if (mode == EInputMode::SHORTCUTS) {
            info.cancelled = true;
            activePlugin()->shortcuts().handleKey(event, monitor);
            return;
        }

        if (activePlugin()->shortcuts().isMenuKey(event)) {
            info.cancelled = true;
            activePlugin()->shortcuts().openMenu(monitor);
            return;
        }

        if (mode == EInputMode::NAMING) {
            info.cancelled = true;
            activePlugin()->naming().handleKey(event, monitor);
            return;
        }

        if (mode == EInputMode::FILTER) {
            info.cancelled = true;
            activePlugin()->workspaceFilter().handleKey(event, monitor);
            return;
        }

        if (mode == EInputMode::CONFIRMATION) {
            info.cancelled = true;
            activePlugin()->confirmation().handleKey(event, monitor);
            return;
        }

        if (activePlugin()->overviewKeyboard().handleKey(event, monitor))
            info.cancelled = true;
    }

    void onMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        activePlugin()->inputRouter().onMouseMove(position, info);
    }

    void onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        activePlugin()->inputRouter().onMouseButton(event, info);
    }

    void onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        activePlugin()->inputRouter().onMouseAxis(event, info);
    }

    void onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
        activePlugin()->inputRouter().onKeyboard(event, info);
    }

    void onRenderStage(eRenderStage stage) {
        activePlugin()->overview().onRenderStage(stage);
    }

} // namespace hyprdeck
