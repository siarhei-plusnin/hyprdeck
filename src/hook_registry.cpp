#include "hook_registry.hpp"

#include "input.hpp"

namespace hyprdeck {

    void CHookRegistry::registerHooks() {
        reset();

        m_renderHook      = Event::bus()->m_events.render.stage.listen(onRenderStage);
        m_mouseMoveHook   = Event::bus()->m_events.input.mouse.move.listen(onMouseMove);
        m_mouseButtonHook = Event::bus()->m_events.input.mouse.button.listen(onMouseButton);
        m_mouseAxisHook   = Event::bus()->m_events.input.mouse.axis.listen(onMouseAxis);
        m_keyboardHook    = Event::bus()->m_events.input.keyboard.key.listen(onKeyboard);
    }

    void CHookRegistry::reset() {
        m_renderHook.reset();
        m_mouseMoveHook.reset();
        m_mouseButtonHook.reset();
        m_mouseAxisHook.reset();
        m_keyboardHook.reset();
    }

} // namespace hyprdeck
