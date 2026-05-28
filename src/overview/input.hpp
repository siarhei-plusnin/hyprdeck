#pragma once

#include <SharedDefs.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <managers/input/InputManager.hpp>

#include "runtime_types.hpp"

namespace hyprdeck {

    class CInputRouter {
      public:
        void       onMouseMove(Vector2D position, Event::SCallbackInfo& info);
        void       onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info);
        void       onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info);
        void       onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info);
        EInputMode activeInputMode() const;

      private:
        PHLMONITOR activeInputMonitor() const;
        bool       inputBlockedByExternalOverlay(const PHLMONITOR& monitor) const;
    };

    void onMouseMove(Vector2D position, Event::SCallbackInfo& info);
    void onMouseButton(IPointer::SButtonEvent event, Event::SCallbackInfo& info);
    void onMouseAxis(IPointer::SAxisEvent event, Event::SCallbackInfo& info);
    void onKeyboard(IKeyboard::SKeyEvent event, Event::SCallbackInfo& info);
    void onRenderStage(eRenderStage stage);

} // namespace hyprdeck
