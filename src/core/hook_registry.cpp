#include "hook_registry.hpp"

#include "SharedDefs.hpp"

namespace hyprdeck {

    void CHookRegistry::registerHooks(const std::function<void(eRenderStage)> renderHook, const std::function<void(Vector2D, Event::SCallbackInfo&)> mouseMoveHook,
                                      const std::function<void(IPointer::SButtonEvent, Event::SCallbackInfo&)> mouseButtonHook,
                                      const std::function<void(IPointer::SAxisEvent, Event::SCallbackInfo&)>   mouseAxisHook,
                                      const std::function<void(IKeyboard::SKeyEvent, Event::SCallbackInfo&)>   keyboardKeyHook) {
        reset();

        m_renderHook      = Event::bus()->m_events.render.stage.listen(renderHook);
        m_mouseMoveHook   = Event::bus()->m_events.input.mouse.move.listen(mouseMoveHook);
        m_mouseButtonHook = Event::bus()->m_events.input.mouse.button.listen(mouseButtonHook);
        m_mouseAxisHook   = Event::bus()->m_events.input.mouse.axis.listen(mouseAxisHook);
        m_keyboardHook    = Event::bus()->m_events.input.keyboard.key.listen(keyboardKeyHook);
    }

    void CHookRegistry::reset() {
        m_renderHook.reset();
        m_mouseMoveHook.reset();
        m_mouseButtonHook.reset();
        m_mouseAxisHook.reset();
        m_keyboardHook.reset();
    }

} // namespace hyprdeck
