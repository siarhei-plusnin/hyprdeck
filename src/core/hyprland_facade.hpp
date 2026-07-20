#pragma once

#include "keyboard.hpp"

#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>
#include <helpers/Color.hpp>
#include <helpers/math/Math.hpp>
#include <render/Texture.hpp>
#include <render/pass/RectPassElement.hpp>
#include <render/pass/RendererHintsPassElement.hpp>
#include <render/pass/TexPassElement.hpp>

#include <string>
#include <vector>

class CEventLoopTimer;

namespace hyprdeck {

    class CHyprlandFacade {
      public:
        PHLMONITOR monitorFromID(MONITORID monitorID) const;
        PHLMONITOR monitorFromCursor() const;
        PHLMONITOR activeMonitor() const;
        PHLMONITOR renderMonitor() const;

        void damageMonitor(const PHLMONITOR& monitor) const;
        void scheduleAnimationFrame(const PHLMONITOR& monitor) const;
        void setCursorHidden(bool hidden) const;

        void lockSoftwarePointer(const PHLMONITOR& monitor) const;
        void unlockSoftwarePointer(const PHLMONITOR& monitor) const;
        bool softwarePointerLockedFor(const PHLMONITOR& monitor) const;
        void damageCursor(const PHLMONITOR& monitor) const;

        Vector2D mouseCoords() const;
        Vector2D pointerPosition() const;
        SP<Render::ITexture> currentCursorTexture() const;
        Vector2D             currentCursorHotspot() const;
        Vector2D             cursorSizeLogical() const;

        const std::vector<PHLWINDOW>& windows() const;
        std::vector<PHLWORKSPACE>     workspacesCopy() const;
        PHLWORKSPACE                  workspaceByID(WORKSPACEID id) const;
        PHLWORKSPACE                  workspaceByName(const std::string& name) const;
        WORKSPACEID                   newSpecialWorkspaceID() const;
        bool                          isSpecialWorkspaceID(WORKSPACEID id) const;
        PHLWORKSPACE                  createWorkspace(WORKSPACEID id, MONITORID monitorID, const std::string& name, bool isEmpty) const;
        void                          postWorkspaceRenameEvent(const PHLWORKSPACE& workspace) const;

        SKeyboardModifiers keyboardModifiers() const;
        int                keyboardRepeatRate() const;
        int                keyboardRepeatDelay() const;

        void addTimer(const SP<CEventLoopTimer>& timer) const;
        void removeTimer(const SP<CEventLoopTimer>& timer) const;

        void                 addRectPass(CRectPassElement::SRectData data) const;
        void                 addRendererHintsPass(CRendererHintsPassElement::SData data) const;
        void                 addTexturePass(CTexPassElement::SRenderData data) const;
        SP<Render::ITexture> renderText(const std::string& text, const CHyprColor& color, int fontSize, const std::string& fontFamily, int weight) const;
    };

} // namespace hyprdeck
