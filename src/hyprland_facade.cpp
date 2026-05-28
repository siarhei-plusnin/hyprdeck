#include "hyprland_facade.hpp"

#include <Compositor.hpp>
#include <desktop/state/FocusState.hpp>
#include <desktop/Workspace.hpp>
#include <devices/IKeyboard.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <managers/EventManager.hpp>
#include <managers/PointerManager.hpp>
#include <managers/SeatManager.hpp>
#include <managers/eventLoop/EventLoopManager.hpp>
#include <managers/eventLoop/EventLoopTimer.hpp>
#include <managers/input/InputManager.hpp>
#include <render/Renderer.hpp>

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
        }

    } // namespace

    PHLMONITOR CHyprlandFacade::monitorFromID(const MONITORID monitorID) const {
        return g_pCompositor ? g_pCompositor->getMonitorFromID(monitorID) : nullptr;
    }

    PHLMONITOR CHyprlandFacade::monitorFromCursor() const {
        return g_pCompositor ? g_pCompositor->getMonitorFromCursor() : nullptr;
    }

    PHLMONITOR CHyprlandFacade::activeMonitor() const {
        if (const auto focusMonitor = Desktop::focusState() ? Desktop::focusState()->monitor() : nullptr; focusMonitor)
            return focusMonitor;

        return monitorFromCursor();
    }

    PHLMONITOR CHyprlandFacade::renderMonitor() const {
        return g_pHyprRenderer ? g_pHyprRenderer->m_renderData.pMonitor.lock() : nullptr;
    }

    void CHyprlandFacade::damageMonitor(const PHLMONITOR& monitor) const {
        if (g_pHyprRenderer && monitor)
            g_pHyprRenderer->damageMonitor(monitor);
    }

    void CHyprlandFacade::setCursorHidden(const bool hidden) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->setCursorHidden(hidden);
    }

    void CHyprlandFacade::lockSoftwarePointer(const PHLMONITOR& monitor) const {
        if (g_pPointerManager && monitor)
            g_pPointerManager->lockSoftwareForMonitor(monitor);
    }

    void CHyprlandFacade::unlockSoftwarePointer(const PHLMONITOR& monitor) const {
        if (g_pPointerManager && monitor)
            g_pPointerManager->unlockSoftwareForMonitor(monitor);
    }

    bool CHyprlandFacade::softwarePointerLockedFor(const PHLMONITOR& monitor) const {
        return g_pPointerManager && monitor && g_pPointerManager->softwareLockedFor(monitor);
    }

    void CHyprlandFacade::damageCursor(const PHLMONITOR& monitor) const {
        if (g_pPointerManager && monitor)
            g_pPointerManager->damageCursor(monitor);
    }

    Vector2D CHyprlandFacade::mouseCoords() const {
        return g_pInputManager ? g_pInputManager->getMouseCoordsInternal() : Vector2D{};
    }

    Vector2D CHyprlandFacade::pointerPosition() const {
        return g_pPointerManager ? g_pPointerManager->position() : Vector2D{};
    }

    SP<Render::ITexture> CHyprlandFacade::currentCursorTexture() const {
        return g_pPointerManager ? g_pPointerManager->getCurrentCursorTexture() : nullptr;
    }

    Vector2D CHyprlandFacade::currentCursorHotspot() const {
        return g_pPointerManager ? g_pPointerManager->currentCursorImage().hotspot : Vector2D{};
    }

    Vector2D CHyprlandFacade::cursorSizeLogical() const {
        return g_pPointerManager ? g_pPointerManager->cursorSizeLogical() : Vector2D{};
    }

    const std::vector<PHLWINDOW>& CHyprlandFacade::windows() const {
        return g_pCompositor ? g_pCompositor->m_windows : emptyWindows();
    }

    std::vector<PHLWORKSPACE> CHyprlandFacade::workspacesCopy() const {
        return g_pCompositor ? g_pCompositor->getWorkspacesCopy() : std::vector<PHLWORKSPACE>{};
    }

    PHLWORKSPACE CHyprlandFacade::workspaceByID(const WORKSPACEID id) const {
        return g_pCompositor ? g_pCompositor->getWorkspaceByID(id) : nullptr;
    }

    PHLWORKSPACE CHyprlandFacade::workspaceByName(const std::string& name) const {
        return g_pCompositor ? g_pCompositor->getWorkspaceByName(name) : nullptr;
    }

    WORKSPACEID CHyprlandFacade::newSpecialWorkspaceID() const {
        return g_pCompositor ? g_pCompositor->getNewSpecialID() : WORKSPACE_INVALID;
    }

    bool CHyprlandFacade::isSpecialWorkspaceID(const WORKSPACEID id) const {
        return g_pCompositor && g_pCompositor->isWorkspaceSpecial(id);
    }

    PHLWORKSPACE CHyprlandFacade::createWorkspace(const WORKSPACEID id, const MONITORID monitorID, const std::string& name, const bool persistent) const {
        return g_pCompositor ? g_pCompositor->createNewWorkspace(id, monitorID, name, persistent) : nullptr;
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

    void CHyprlandFacade::addTexturePass(CTexPassElement::SRenderData data) const {
        if (g_pHyprRenderer)
            g_pHyprRenderer->m_renderPass.add(makeUnique<CTexPassElement>(std::move(data)));
    }

    SP<Render::ITexture> CHyprlandFacade::renderText(const std::string& text, const CHyprColor& color, const int fontSize, const std::string& fontFamily, const int weight) const {
        return g_pHyprRenderer ? g_pHyprRenderer->renderText(text, color, fontSize, false, fontFamily, 0, weight) : nullptr;
    }

} // namespace hyprdeck
