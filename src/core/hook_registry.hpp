#pragma once

#include <event/EventBus.hpp>

namespace hyprdeck {

    class CHookRegistry {
      public:
        void registerHooks(const std::function<void(eRenderStage)> renderHook, const std::function<void(Vector2D, Event::SCallbackInfo&)> mouseMoveHook,
                           const std::function<void(IPointer::SButtonEvent, Event::SCallbackInfo&)> mouseButtonHook,
                           const std::function<void(IPointer::SAxisEvent, Event::SCallbackInfo&)>   mouseAxisHook,
                           const std::function<void(IKeyboard::SKeyEvent, Event::SCallbackInfo&)> keyboardKeyHook, const std::function<void()>& stateChangedHook,
                           const std::function<void(PHLMONITOR)>& monitorRemovedHook);
        void reset();

      private:
        CHyprSignalListener m_renderHook;
        CHyprSignalListener m_mouseMoveHook;
        CHyprSignalListener m_mouseButtonHook;
        CHyprSignalListener m_mouseAxisHook;
        CHyprSignalListener m_keyboardHook;
        CHyprSignalListener m_monitorAddedHook;
        CHyprSignalListener m_monitorRemovedHook;
        CHyprSignalListener m_monitorRemovedStateHook;
        CHyprSignalListener m_monitorLayoutHook;
        CHyprSignalListener m_workspaceMoveHook;
        CHyprSignalListener m_workspaceActiveHook;
        CHyprSignalListener m_workspaceSpecialActiveHook;
        CHyprSignalListener m_workspaceCreatedHook;
        CHyprSignalListener m_workspaceRemovedHook;
        CHyprSignalListener m_windowOpenHook;
        CHyprSignalListener m_windowCloseHook;
        CHyprSignalListener m_windowDestroyHook;
        CHyprSignalListener m_windowMoveHook;
        CHyprSignalListener m_windowTitleHook;
        CHyprSignalListener m_windowClassHook;
        CHyprSignalListener m_windowPinHook;
    };

} // namespace hyprdeck
