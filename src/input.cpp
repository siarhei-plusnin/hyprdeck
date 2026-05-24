#include "input.hpp"

#include "monitors.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "overview_controller.hpp"
#include "overview_interaction.hpp"
#include "shortcuts.hpp"
#include "state.hpp"

#include <helpers/Monitor.hpp>

namespace hyprdeck {

    void onMouseMove(Vector2D position, Event::SCallbackInfo& info) {
        handleOverviewMouseMove(position, info);
    }

    void onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        handleOverviewMouseButton(event, info);
    }

    void onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
        handleOverviewMouseAxis(event, info);
    }

    void onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
        const auto mode = currentInputMode();
        if (mode == EInputMode::INACTIVE)
            return;

        const auto monitor = overviewMonitor();
        if (!monitor) {
            closeOverview();
            return;
        }

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
