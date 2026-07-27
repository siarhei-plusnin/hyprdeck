#include "confirmation.hpp"

#include "colors.hpp"
#include "layout.hpp"
#include "navigation.hpp"
#include "plugin.hpp"
#include "runtime_types.hpp"
#include "workspaces.hpp"

#include <desktop/Workspace.hpp>
#include <output/Monitor.hpp>
#include <render/Renderer.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <linux/input-event-codes.h>
#include <string>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        void damageConfirmation(const PHLMONITOR& monitor) {
            activePlugin()->overview().damageHost();
        }

        SP<Render::ITexture> confirmationTexture(const std::string& text, const int fontSize, const int weight, const CHyprColor& color = colors::textPrimary()) {
            return activePlugin()->renderServices().textTexture("confirmation", text, color, fontSize, weight, ETextCacheMode::NONE);
        }

        void renderBox(const CBox& box) {
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(box, 2.0), colors::componentBorder());
            activePlugin()->renderServices().addRect(box, colors::componentBackground());
        }

        void renderButton(const CBox& box, const std::string& key, const std::string& label) {
            activePlugin()->renderServices().addRect(box, colors::componentSurface());

            const auto keyTexture   = confirmationTexture(key, 17, 750);
            const auto labelTexture = confirmationTexture(label, 17, 600, colors::textSecondary());
            if (!keyTexture || !keyTexture->ok() || !labelTexture || !labelTexture->ok())
                return;

            const double gap      = 8.0;
            const double contentW = keyTexture->m_size.x + gap + labelTexture->m_size.x;
            const double x        = box.x + ((box.w - contentW) / 2.0);
            activePlugin()->renderServices().addTexture(keyTexture, CBox{x, box.y + ((box.h - keyTexture->m_size.y) / 2.0), keyTexture->m_size.x, keyTexture->m_size.y});
            activePlugin()->renderServices().addTexture(
                labelTexture, CBox{x + keyTexture->m_size.x + gap, box.y + ((box.h - labelTexture->m_size.y) / 2.0), labelTexture->m_size.x, labelTexture->m_size.y});
        }

    } // namespace

    bool CConfirmationController::promptOpen() const {
        return m_state.open;
    }

    void CConfirmationController::openCloseNormalWorkspaceConfirmation(const WORKSPACEID workspaceID, const PHLMONITOR& monitor) {
        if (workspaceID == WORKSPACE_INVALID)
            return;

        auto& confirmation             = m_state;
        confirmation.open              = true;
        confirmation.normalWorkspaceID = workspaceID;
        damageConfirmation(monitor);
    }

    void CConfirmationController::closePrompt(const PHLMONITOR& monitor) {
        if (!m_state.open)
            return;

        resetState();
        damageConfirmation(monitor);
    }

    void CConfirmationController::resetState() {
        auto& confirmation             = m_state;
        confirmation.open              = false;
        confirmation.normalWorkspaceID = WORKSPACE_INVALID;
    }

    void CConfirmationController::handleKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!m_state.open || event.state != WL_KEYBOARD_KEY_STATE_PRESSED)
            return;

        if (event.keycode == KEY_ESC) {
            closePrompt(monitor);
            return;
        }

        if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
            confirmCloseNormalWorkspace(monitor);
    }

    void CConfirmationController::confirmCloseNormalWorkspace(const PHLMONITOR& monitor) {
        auto& confirmation = m_state;
        if (!confirmation.open)
            return;

        const auto workspace        = activePlugin()->hyprland().workspaceByID(confirmation.normalWorkspaceID);
        const auto workspaceMonitor = activePlugin()->workspaces().workspaceMonitor(workspace);
        resetState();

        if (!activePlugin()->workspaces().isNormalWorkspace(workspace) || !workspaceMonitor) {
            damageConfirmation(monitor);
            return;
        }

        activePlugin()->navigator().closeWorkspaceWindows(workspace, workspaceMonitor);
        activePlugin()->layout().invalidate();
        damageConfirmation(monitor);
    }

    void CConfirmationController::render(const PHLMONITOR& monitor) {
        const auto& confirmation = m_state;
        if (!confirmation.open)
            return;

        const auto workspace = activePlugin()->hyprland().workspaceByID(confirmation.normalWorkspaceID);
        const auto label     = activePlugin()->workspaces().isNormalWorkspace(workspace) ? activePlugin()->workspaces().normalWorkspaceLabel(workspace) :
                                                                                           std::to_string(confirmation.normalWorkspaceID);
        const auto title     = confirmationTexture("Close all windows?", 22, 750);
        const auto subtitle  = confirmationTexture("Normal workspace " + label, 18, 600, colors::textSecondary());
        if (!title || !title->ok() || !subtitle || !subtitle->ok())
            return;

        const auto   viewSize = monitor->m_transformedSize;
        const double boxW     = std::min(460.0, viewSize.x - 48.0);
        const double boxH     = 150.0;
        const CBox   box{(viewSize.x - boxW) / 2.0, (viewSize.y - boxH) / 2.0, boxW, boxH};
        renderBox(box);

        activePlugin()->renderServices().addTexture(title, CBox{box.x + 24.0, box.y + 22.0, title->m_size.x, title->m_size.y});
        activePlugin()->renderServices().addTexture(subtitle, CBox{box.x + 24.0, box.y + 56.0, subtitle->m_size.x, subtitle->m_size.y});

        const double buttonW = (box.w - 58.0) / 2.0;
        const double buttonH = 36.0;
        const double buttonY = box.y + box.h - buttonH - 22.0;
        renderButton(CBox{box.x + 24.0, buttonY, buttonW, buttonH}, "Enter", "Close");
        renderButton(CBox{box.x + 34.0 + buttonW, buttonY, buttonW, buttonH}, "Esc", "Cancel");
    }

} // namespace hyprdeck
