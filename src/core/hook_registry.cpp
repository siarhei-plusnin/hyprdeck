#include "hook_registry.hpp"

#include "SharedDefs.hpp"

namespace hyprdeck {

    void CHookRegistry::registerHooks(const std::function<void(eRenderStage)> renderHook, const std::function<void(Vector2D, Event::SCallbackInfo&)> mouseMoveHook,
                                      const std::function<void(IPointer::SButtonEvent, Event::SCallbackInfo&)> mouseButtonHook,
                                      const std::function<void(IPointer::SAxisEvent, Event::SCallbackInfo&)>   mouseAxisHook,
                                      const std::function<void(IKeyboard::SKeyEvent, Event::SCallbackInfo&)> keyboardKeyHook, const std::function<void()>& stateChangedHook,
                                      const std::function<void(PHLMONITOR)>& monitorRemovedHook) {
        reset();

        m_renderHook                 = Event::bus()->m_events.render.stage.listen(renderHook);
        m_mouseMoveHook              = Event::bus()->m_events.input.mouse.move.listen(mouseMoveHook);
        m_mouseButtonHook            = Event::bus()->m_events.input.mouse.button.listen(mouseButtonHook);
        m_mouseAxisHook              = Event::bus()->m_events.input.mouse.axis.listen(mouseAxisHook);
        m_keyboardHook               = Event::bus()->m_events.input.keyboard.key.listen(keyboardKeyHook);
        m_monitorAddedHook           = Event::bus()->m_events.monitor.added.listen([stateChangedHook](PHLMONITOR) { stateChangedHook(); });
        m_monitorRemovedHook         = Event::bus()->m_events.monitor.preRemoved.listen([stateChangedHook, monitorRemovedHook](PHLMONITOR monitor) {
            monitorRemovedHook(monitor);
            stateChangedHook();
        });
        m_monitorRemovedStateHook    = Event::bus()->m_events.monitor.removed.listen([stateChangedHook](PHLMONITOR) { stateChangedHook(); });
        m_monitorLayoutHook          = Event::bus()->m_events.monitor.layoutChanged.listen(stateChangedHook);
        m_workspaceMoveHook          = Event::bus()->m_events.workspace.moveToMonitor.listen([stateChangedHook](PHLWORKSPACE, PHLMONITOR) { stateChangedHook(); });
        m_workspaceActiveHook        = Event::bus()->m_events.workspace.active.listen([stateChangedHook](PHLWORKSPACE) { stateChangedHook(); });
        m_workspaceSpecialActiveHook = Event::bus()->m_events.workspace.specialActive.listen([stateChangedHook](PHLWORKSPACE, PHLMONITOR) { stateChangedHook(); });
        m_workspaceCreatedHook       = Event::bus()->m_events.workspace.created.listen([stateChangedHook](PHLWORKSPACEREF) { stateChangedHook(); });
        m_workspaceRemovedHook       = Event::bus()->m_events.workspace.removed.listen([stateChangedHook](PHLWORKSPACEREF) { stateChangedHook(); });
        m_windowOpenHook             = Event::bus()->m_events.window.open.listen([stateChangedHook](PHLWINDOW) { stateChangedHook(); });
        m_windowCloseHook            = Event::bus()->m_events.window.close.listen([stateChangedHook](PHLWINDOW) { stateChangedHook(); });
        m_windowDestroyHook          = Event::bus()->m_events.window.destroy.listen([stateChangedHook](PHLWINDOWREF) { stateChangedHook(); });
        m_windowMoveHook             = Event::bus()->m_events.window.moveToWorkspace.listen([stateChangedHook](PHLWINDOW, PHLWORKSPACE) { stateChangedHook(); });
        m_windowTitleHook            = Event::bus()->m_events.window.title.listen([stateChangedHook](PHLWINDOW) { stateChangedHook(); });
        m_windowClassHook            = Event::bus()->m_events.window.class_.listen([stateChangedHook](PHLWINDOW) { stateChangedHook(); });
        m_windowPinHook              = Event::bus()->m_events.window.pin.listen([stateChangedHook](PHLWINDOW) { stateChangedHook(); });
    }

    void CHookRegistry::reset() {
        m_renderHook.reset();
        m_mouseMoveHook.reset();
        m_mouseButtonHook.reset();
        m_mouseAxisHook.reset();
        m_keyboardHook.reset();
        m_monitorAddedHook.reset();
        m_monitorRemovedHook.reset();
        m_monitorRemovedStateHook.reset();
        m_monitorLayoutHook.reset();
        m_workspaceMoveHook.reset();
        m_workspaceActiveHook.reset();
        m_workspaceSpecialActiveHook.reset();
        m_workspaceCreatedHook.reset();
        m_workspaceRemovedHook.reset();
        m_windowOpenHook.reset();
        m_windowCloseHook.reset();
        m_windowDestroyHook.reset();
        m_windowMoveHook.reset();
        m_windowTitleHook.reset();
        m_windowClassHook.reset();
        m_windowPinHook.reset();
    }

} // namespace hyprdeck
