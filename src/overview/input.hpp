#pragma once

#include <SharedDefs.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <managers/input/InputManager.hpp>

#include "runtime_types.hpp"

#include <functional>

namespace hyprdeck {

    class CInputRouter {
      public:
        std::function<void(Vector2D, Event::SCallbackInfo&)>               onMouseMove = [this](Vector2D position, Event::SCallbackInfo& info) { handleMouseMove(position, info); };
        std::function<void(IPointer::SButtonEvent, Event::SCallbackInfo&)> onMouseButton = [this](IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
            handleMouseButton(event, info);
        };
        std::function<void(IPointer::SAxisEvent, Event::SCallbackInfo&)> onMouseAxis = [this](IPointer::SAxisEvent event, Event::SCallbackInfo& info) {
            handleMouseAxis(event, info);
        };
        std::function<void(IKeyboard::SKeyEvent, Event::SCallbackInfo&)> onKeyboard = [this](IKeyboard::SKeyEvent event, Event::SCallbackInfo& info) {
            handleKeyboard(event, info);
        };

        EInputMode activeInputMode() const;

      private:
        PHLMONITOR activeHostMonitor() const;
        PHLMONITOR activeSelectedMonitor() const;
        bool       inputBlockedByExternalOverlay(const PHLMONITOR& monitor) const;

        void       handleMouseMove(Vector2D position, Event::SCallbackInfo& info);
        void       handleMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info);
        void       handleMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info);
        void       handleKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info);
    };

} // namespace hyprdeck
