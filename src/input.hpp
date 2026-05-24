#pragma once

#include <SharedDefs.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <managers/input/InputManager.hpp>

namespace hyprdeck {

    void onMouseMove(Vector2D position, Event::SCallbackInfo& info);
    void onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info);
    void onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info);
    void onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info);
    void onRenderStage(eRenderStage stage);
    void resetHooks();

} // namespace hyprdeck
