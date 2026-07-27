#include "hyprland_facade.hpp"

#include <desktop/state/FocusState.hpp>
#include <desktop/state/ViewState.hpp>
#include <desktop/Workspace.hpp>
#include <config/shared/actions/ConfigActions.hpp>
#include <devices/IKeyboard.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <managers/EventManager.hpp>
#include <managers/SeatManager.hpp>
#include <managers/eventLoop/EventLoopManager.hpp>
#include <managers/eventLoop/EventLoopTimer.hpp>
#include <managers/input/InputManager.hpp>
#include <output/Monitor.hpp>
#include <pointer/PointerManager.hpp>
#include <render/Renderer.hpp>
#include <state/MonitorState.hpp>
#include <state/WorkspacePlacementController.hpp>
#include <state/WorkspaceState.hpp>

using Hyprutils::Memory::makeUnique;

namespace hyprdeck {
    namespace {

        const std::vector<PHLWINDOW>& emptyWindows() {
            static const std::vector<PHLWINDOW> EMPTY;
            return EMPTY;
        }

        void applyModifiers(SKeyboardModifiers& modifiers, const uint32_t activeModifiers) {
            modifiers.ctrl  = modifiers.ctrl || (activeModifiers & HL_MODIFIER_CTRL);
            modifiers.shift = modifiers.shift || (activeModifiers & HL_MODIFIER_SHIFT);
            modifiers.super = modifiers.super || (activeModifiers & HL_MODIFIER_META);
            modifiers.alt   = modifiers.alt || (activeModifiers & HL_MODIFIER_ALT);
        }

    } // namespace

    PHLMONITOR CHyprlandFacade::monitorFromID(const MONITORID monitorID) const {
        return State::monitorState() ? State::monitorState()->query().id(monitorID).run() : nullptr;
    }

    PHLMONITOR CHyprlandFacade::monitorFromCursor() const {
        return State::monitorState() ? State::monitorState()->query().vec(mouseCoords()).run() : nullptr;
    }

    PHLMONITOR CHyprlandFacade::activeMonitor() const {
        if (const auto focusMonitor = Desktop::focusState() ? Desktop::focusState()->monitor() : nullptr; focusMonitor)
            return focusMonitor;

        return monitorFromCursor();
    }

    PHLMONITOR CHyprlandFacade::renderMonitor() const {
        return g_pHyprRenderer ? g_pHyprRenderer->m_renderData.pMonitor.lock() : nullptr;
    }

    std::vector<PHLMONITOR> CHyprlandFacade::monitors() const {
        if (!State::monitorState())
            return {};

        std::vector<PHLMONITOR> monitors;
        for (const auto& monitor : State::monitorState()->monitors()) {
            if (monitor && !monitor->m_mirrorOf.lock())
                monitors.push_back(monitor);
        }

        return monitors;
    }

    void CHyprlandFacade::damageMonitor(const PHLMONITOR& monitor) const {
        if (g_pHyprRenderer && monitor)
            g_pHyprRenderer->damageMonitor(monitor);
    }

    void CHyprlandFacade::scheduleAnimationFrame(const PHLMONITOR& monitor) const {
        if (monitor)
            monitor->scheduleFrame(Aquamarine::IOutput::AQ_SCHEDULE_ANIMATION);
    }

    void CHyprlandFacade::setCursorHidden(const bool hidden) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->setCursorHidden(hidden);
    }

    void CHyprlandFacade::lockSoftwarePointer(const PHLMONITOR& monitor) const {
        if (Pointer::mgr() && monitor)
            Pointer::mgr()->lockSoftwareForMonitor(monitor);
    }

    void CHyprlandFacade::unlockSoftwarePointer(const PHLMONITOR& monitor) const {
        if (Pointer::mgr() && monitor)
            Pointer::mgr()->unlockSoftwareForMonitor(monitor);
    }

    bool CHyprlandFacade::softwarePointerLockedFor(const PHLMONITOR& monitor) const {
        return Pointer::mgr() && monitor && Pointer::mgr()->softwareLockedFor(monitor);
    }

    void CHyprlandFacade::damageCursor(const PHLMONITOR& monitor) const {
        if (Pointer::mgr() && monitor)
            Pointer::mgr()->damageCursor(monitor);
    }

    Vector2D CHyprlandFacade::mouseCoords() const {
        return g_pInputManager ? g_pInputManager->getMouseCoordsInternal() : Vector2D{};
    }

    Vector2D CHyprlandFacade::pointerPosition() const {
        return Pointer::mgr() ? Pointer::mgr()->position() : Vector2D{};
    }

    SP<Render::ITexture> CHyprlandFacade::currentCursorTexture() const {
        return Pointer::mgr() ? Pointer::mgr()->getCurrentCursorTexture() : nullptr;
    }

    Vector2D CHyprlandFacade::currentCursorHotspot() const {
        return Pointer::mgr() ? Pointer::mgr()->currentCursorImage().hotspot : Vector2D{};
    }

    Vector2D CHyprlandFacade::cursorSizeLogical() const {
        return Pointer::mgr() ? Pointer::mgr()->cursorSizeLogical() : Vector2D{};
    }

    const std::vector<PHLWINDOW>& CHyprlandFacade::windows() const {
        return Desktop::viewState() ? Desktop::viewState()->windows() : emptyWindows();
    }

    std::vector<PHLWORKSPACE> CHyprlandFacade::workspacesCopy() const {
        return State::workspaceState() ? State::workspaceState()->workspacesCopy() : std::vector<PHLWORKSPACE>{};
    }

    PHLWORKSPACE CHyprlandFacade::workspaceByID(const WORKSPACEID id) const {
        return State::workspaceState() ? State::workspaceState()->query().id(id).run() : nullptr;
    }

    PHLWORKSPACE CHyprlandFacade::workspaceByName(const std::string& name) const {
        return State::workspaceState() ? State::workspaceState()->query().name(name).run() : nullptr;
    }

    WORKSPACEID CHyprlandFacade::newSpecialWorkspaceID() const {
        return State::workspaceState() ? State::workspaceState()->newSpecialID() : WORKSPACE_INVALID;
    }

    bool CHyprlandFacade::isSpecialWorkspaceID(const WORKSPACEID id) const {
        return State::workspaceState() && State::workspaceState()->isSpecial(id);
    }

    PHLWORKSPACE CHyprlandFacade::createWorkspace(const WORKSPACEID id, const MONITORID monitorID, const std::string& name, const bool isEmpty) const {
        return State::workspaceState() ? State::workspaceState()->create(id, monitorID, name, isEmpty) : nullptr;
    }

    void CHyprlandFacade::moveWorkspaceToMonitor(const PHLWORKSPACE& workspace, const PHLMONITOR& monitor) const {
        if (workspace && monitor && State::workspacePlacementController())
            State::workspacePlacementController()->moveWorkspaceToMonitor(workspace, monitor, true);
    }

    void CHyprlandFacade::focusMonitor(const PHLMONITOR& monitor) const {
        if (monitor)
            static_cast<void>(Config::Actions::focusMonitor(monitor));
    }

    void CHyprlandFacade::postWorkspaceRenameEvent(const PHLWORKSPACE& workspace) const {
        if (g_pEventManager && workspace)
            g_pEventManager->postEvent({.event = "renameworkspace", .data = std::to_string(workspace->m_id) + "," + workspace->m_name});
    }

    SKeyboardModifiers CHyprlandFacade::keyboardModifiers() const {
        SKeyboardModifiers modifiers;

        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard)
            applyModifiers(modifiers, keyboard->getModifiers());

        if (g_pInputManager) {
            for (const auto& keyboard : g_pInputManager->m_keyboards) {
                if (keyboard)
                    applyModifiers(modifiers, keyboard->getModifiers());
            }
        }

        return modifiers;
    }

    int CHyprlandFacade::keyboardRepeatRate() const {
        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard)
            return keyboard->m_repeatRate;

        return 25;
    }

    int CHyprlandFacade::keyboardRepeatDelay() const {
        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard)
            return keyboard->m_repeatDelay;

        return 600;
    }

    void CHyprlandFacade::addTimer(const SP<CEventLoopTimer>& timer) const {
        if (g_pEventLoopManager && timer)
            g_pEventLoopManager->addTimer(timer);
    }

    void CHyprlandFacade::removeTimer(const SP<CEventLoopTimer>& timer) const {
        if (g_pEventLoopManager && timer)
            g_pEventLoopManager->removeTimer(timer);
    }

    void CHyprlandFacade::addRectPass(CRectPassElement::SRectData data) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->m_renderPass.add(makeUnique<CRectPassElement>(std::move(data)));
    }

    void CHyprlandFacade::addRendererHintsPass(CRendererHintsPassElement::SData data) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->m_renderPass.add(makeUnique<CRendererHintsPassElement>(std::move(data)));
    }

    void CHyprlandFacade::addTexturePass(CTexPassElement::SRenderData data) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->m_renderPass.add(makeUnique<CTexPassElement>(std::move(data)));
    }

    SP<Render::ITexture> CHyprlandFacade::renderText(const std::string& text, const CHyprColor& color, const int fontSize, const std::string& fontFamily, const int weight) const {
        return g_pHyprRenderer ? g_pHyprRenderer->renderText(text, color, fontSize, false, fontFamily, 0, weight) : nullptr;
    }

} // namespace hyprdeck
