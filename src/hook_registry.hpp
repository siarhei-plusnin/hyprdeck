#pragma once

#include <event/EventBus.hpp>

namespace hyprdeck {

    class CHookRegistry {
      public:
        void registerHooks();
        void reset();

      private:
        CHyprSignalListener m_renderHook;
        CHyprSignalListener m_mouseMoveHook;
        CHyprSignalListener m_mouseButtonHook;
        CHyprSignalListener m_mouseAxisHook;
        CHyprSignalListener m_keyboardHook;
    };

} // namespace hyprdeck
