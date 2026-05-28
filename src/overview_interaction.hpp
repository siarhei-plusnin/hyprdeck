#pragma once

#include <SharedDefs.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <managers/input/InputManager.hpp>

#include "runtime_types.hpp"

namespace hyprdeck {

    class COverviewPointerController {
      public:
        void handleMouseMove(Vector2D position, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
        void handleMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
        void handleMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info, const PHLMONITOR& monitor);
        void resetState();

      private:
        SInteractionState m_state;
    };

} // namespace hyprdeck
