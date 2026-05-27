#pragma once

#include <SharedDefs.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <managers/input/InputManager.hpp>

namespace hyprdeck {

    void handleOverviewMouseMove(Vector2D position, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
    void handleOverviewMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
    void handleOverviewMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
    void handleOverviewRenderStage(eRenderStage stage);

} // namespace hyprdeck
