#pragma once

#include <event/EventBus.hpp>

namespace hyprdeck {

    class CHookRegistry {
      public:
        void registerHooks(const std::function<void(eRenderStage)> renderHook, const std::function<void(Vector2D, Event::SCallbackInfo&)> mouseMoveHook,
                           const std::function<void(IPointer::SButtonEvent, Event::SCallbackInfo&)> mouseButtonHook,
                           const std::function<void(IPointer::SAxisEvent, Event::SCallbackInfo&)>   mouseAxisHook,
                           const std::function<void(IKeyboard::SKeyEvent, Event::SCallbackInfo&)>   keyboardKeyHook);
        void reset();

      private:
        CHyprSignalListener m_renderHook;
        CHyprSignalListener m_mouseMoveHook;
        CHyprSignalListener m_mouseButtonHook;
        CHyprSignalListener m_mouseAxisHook;
        CHyprSignalListener m_keyboardHook;
    };

} // namespace hyprdeck
