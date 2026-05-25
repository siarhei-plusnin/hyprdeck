#include "confirmation.hpp"

#include "colors.hpp"
#include "layout.hpp"
#include "navigation.hpp"
#include "state.hpp"
#include "ui.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <desktop/Workspace.hpp>
#include <helpers/Monitor.hpp>
#include <render/Renderer.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <linux/input-event-codes.h>
#include <string>
#include <wayland-server-protocol.h>

namespace hyprdeck {
    namespace {

        void damageConfirmation(const PHLMONITOR& monitor) {
            if (monitor)
                g_pHyprRenderer->damageMonitor(monitor);
        }

        SP<Render::ITexture> confirmationTexture(const std::string& text, const int fontSize, const int weight, const CHyprColor& color = colors::textPrimary()) {
            return textTexture("confirmation", text, color, fontSize, weight, ETextCacheMode::NONE);
        }

        void renderBox(const CBox& box) {
            addRect(expanded(box, 2.0), colors::componentBorder());
            addRect(box, colors::componentBackground());
        }

        void renderButton(const CBox& box, const std::string& key, const std::string& label) {
            addRect(box, colors::componentSurface());

            const auto keyTexture   = confirmationTexture(key, 17, 750);
            const auto labelTexture = confirmationTexture(label, 17, 600, colors::textSecondary());
            if (!keyTexture || !keyTexture->ok() || !labelTexture || !labelTexture->ok())
                return;

            const double gap      = 8.0;
            const double contentW = keyTexture->m_size.x + gap + labelTexture->m_size.x;
            const double x        = box.x + ((box.w - contentW) / 2.0);
            addTexture(keyTexture, CBox{x, box.y + ((box.h - keyTexture->m_size.y) / 2.0), keyTexture->m_size.x, keyTexture->m_size.y});
            addTexture(labelTexture, CBox{x + keyTexture->m_size.x + gap, box.y + ((box.h - labelTexture->m_size.y) / 2.0), labelTexture->m_size.x, labelTexture->m_size.y});
        }

        void confirmCloseNormalWorkspace(const PHLMONITOR& monitor) {
            auto& confirmation = state().confirmation;
            if (!confirmation.open)
                return;

            const auto workspace = g_pCompositor->getWorkspaceByID(confirmation.normalWorkspaceID);
            resetConfirmationState();

            if (!isNormalNumericWorkspace(workspace)) {
                damageConfirmation(monitor);
                return;
            }

            closeWorkspaceWindows(workspace, monitor);
            invalidateLayout();
            damageConfirmation(monitor);
        }

    } // namespace

    bool confirmationPromptOpen() {
        return state().confirmation.open;
    }

    void openCloseNormalWorkspaceConfirmation(const WORKSPACEID workspaceID, const PHLMONITOR& monitor) {
        if (workspaceID == WORKSPACE_INVALID)
            return;

        auto& confirmation              = state().confirmation;
        confirmation.open               = true;
        confirmation.normalWorkspaceID  = workspaceID;
        damageConfirmation(monitor);
    }

    void closeConfirmationPrompt(const PHLMONITOR& monitor) {
        if (!state().confirmation.open)
            return;

        resetConfirmationState();
        damageConfirmation(monitor);
    }

    void resetConfirmationState() {
        auto& confirmation             = state().confirmation;
        confirmation.open              = false;
        confirmation.normalWorkspaceID = WORKSPACE_INVALID;
    }

    void handleConfirmationKey(const IKeyboard::SKeyEvent event, const PHLMONITOR& monitor) {
        if (!state().confirmation.open || event.state != WL_KEYBOARD_KEY_STATE_PRESSED)
            return;

        if (event.keycode == KEY_ESC) {
            closeConfirmationPrompt(monitor);
            return;
        }

        if (event.keycode == KEY_ENTER || event.keycode == KEY_KPENTER)
            confirmCloseNormalWorkspace(monitor);
    }

    void renderConfirmationPrompt(const PHLMONITOR& monitor) {
        const auto& confirmation = state().confirmation;
        if (!monitor || !confirmation.open)
            return;

        const auto title    = confirmationTexture("Close all windows?", 22, 750);
        const auto subtitle = confirmationTexture("Normal workspace " + std::to_string(confirmation.normalWorkspaceID), 18, 600, colors::textSecondary());
        if (!title || !title->ok() || !subtitle || !subtitle->ok())
            return;

        const auto   viewSize = monitor->m_transformedSize;
        const double boxW     = std::min(460.0, viewSize.x - 48.0);
        const double boxH     = 150.0;
        const CBox   box{(viewSize.x - boxW) / 2.0, (viewSize.y - boxH) / 2.0, boxW, boxH};
        renderBox(box);

        addTexture(title, CBox{box.x + 24.0, box.y + 22.0, title->m_size.x, title->m_size.y});
        addTexture(subtitle, CBox{box.x + 24.0, box.y + 56.0, subtitle->m_size.x, subtitle->m_size.y});

        const double buttonW = (box.w - 58.0) / 2.0;
        const double buttonH = 36.0;
        const double buttonY = box.y + box.h - buttonH - 22.0;
        renderButton(CBox{box.x + 24.0, buttonY, buttonW, buttonH}, "Enter", "Close");
        renderButton(CBox{box.x + 34.0 + buttonW, buttonY, buttonW, buttonH}, "Esc", "Cancel");
    }

} // namespace hyprdeck
