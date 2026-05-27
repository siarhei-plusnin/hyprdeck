#include "input.hpp"

#include "confirmation.hpp"
#include "monitors.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "overview_controller.hpp"
#include "overview_interaction.hpp"
#include "overlays.hpp"
#include "shortcuts.hpp"
#include "state.hpp"
#include "workspace_filter.hpp"

namespace hyprdeck {
    namespace {

        PHLMONITOR activeInputMonitor() {
            if (!state().session.active)
                return nullptr;

            const auto monitor = overviewMonitor();
            if (!monitor)
                closeOverview();

            return monitor;
        }

        bool inputBlockedByExternalOverlay(const PHLMONITOR& monitor) {
            return externalOverlayActive(monitor);
        }

    } // namespace

    void onMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        handleOverviewMouseMove(position, info, monitor);
    }

    void onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        handleOverviewMouseButton(event, info, monitor);
    }

    void onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        handleOverviewMouseAxis(event, info, monitor);
    }

    void onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
        const auto monitor = activeInputMonitor();
        if (!monitor || inputBlockedByExternalOverlay(monitor))
            return;

        const auto mode = currentInputMode();
        if (mode == EInputMode::INACTIVE)
            return;

        if (mode == EInputMode::SHORTCUTS) {
            info.cancelled = true;
            handleShortcutMenuKey(event, monitor);
            return;
        }

        if (isShortcutMenuKey(event)) {
            info.cancelled = true;
            openShortcutMenu(monitor);
            return;
        }

        if (mode == EInputMode::NAMING) {
            info.cancelled = true;
            handleNamingPromptKey(event, monitor);
            return;
        }

        if (mode == EInputMode::FILTER) {
            info.cancelled = true;
            handleWorkspaceFilterKey(event, monitor);
            return;
        }

        if (mode == EInputMode::CONFIRMATION) {
            info.cancelled = true;
            handleConfirmationKey(event, monitor);
            return;
        }

        if (handleOverviewKeyboardKey(event, monitor))
            info.cancelled = true;
    }

    void onRenderStage(eRenderStage stage) {
        handleOverviewRenderStage(stage);
    }

    void resetHooks() {
        auto& current = state();
        current.hooks.renderHook.reset();
        current.hooks.mouseMoveHook.reset();
        current.hooks.mouseButtonHook.reset();
        current.hooks.mouseAxisHook.reset();
        current.hooks.keyboardHook.reset();
    }

} // namespace hyprdeck
