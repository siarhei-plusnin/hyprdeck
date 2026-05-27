#include "overview.hpp"

#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "monitors.hpp"
#include "naming.hpp"
#include "shortcuts.hpp"
#include "state.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <helpers/Monitor.hpp>
#include <managers/PointerManager.hpp>
#include <render/Renderer.hpp>

namespace hyprdeck {
    namespace {

        void resetInteractionState(SOverviewState& current) {
            auto& interaction                = current.interaction;
            interaction.dragging             = false;
            interaction.dragRow              = EDragRow::NONE;
            resetNamingPromptState();
            resetWorkspaceFilterPromptState();
            resetConfirmationState();
            resetShortcutState();
        }

        void resetSelectionState(SOverviewState& current) {
            auto& selection                = current.selection;
            selection.selectedRow          = ESelectedRow::NORMAL;
            selection.selectedNormalID     = 1;
            selection.selectedSpecialID    = WORKSPACE_INVALID;
            selection.lastActiveNormalID   = WORKSPACE_INVALID;
            selection.lastActiveSpecialID  = WORKSPACE_INVALID;
        }

    } // namespace

    void closeOverview() {
        auto& current = state();
        if (!current.session.active)
            return;

        const auto monitor = overviewMonitor();

        current.session.active    = false;
        current.session.monitorID = MONITOR_INVALID;
        current.layout.cards.clear();
        current.layout.specialCards.clear();
        invalidateLayout();
        resetInteractionState(current);
        resetSelectionState(current);

        if (monitor) {
            g_pPointerManager->unlockSoftwareForMonitor(monitor);
            g_pHyprRenderer->damageMonitor(monitor);
        }
    }

    void openOverview() {
        const auto monitor = activeMonitor();
        if (!monitor)
            return;

        auto& current = state();
        if (!current.session.zoomInitialized) {
            current.session.zoom            = configuredDefaultZoom();
            current.session.zoomInitialized = true;
        }

        auto& session                  = current.session;
        auto& layout                   = current.layout;
        auto& selection                = current.selection;
        session.active                 = true;
        layout.resetCamera             = true;
        session.monitorID              = monitor->m_id;
        selection.selectedNormalID     = activeNormalWorkspaceID(monitor);
        selection.selectedSpecialID    = activeSpecialWorkspaceID(monitor);
        selection.selectedRow          = selection.selectedSpecialID != WORKSPACE_INVALID ? ESelectedRow::SPECIAL : ESelectedRow::NORMAL;
        selection.lastActiveNormalID   = selection.selectedNormalID;
        selection.lastActiveSpecialID  = selection.selectedSpecialID;
        resetInteractionState(current);
        invalidateLayout();

        g_pPointerManager->lockSoftwareForMonitor(monitor);

        recalculateCards(monitor);
        g_pHyprRenderer->damageMonitor(monitor);
    }

    SDispatchResult toggleOverview(std::string) {
        if (state().session.active)
            closeOverview();
        else
            openOverview();

        return SDispatchResult{.passEvent = false, .success = true};
    }

    int luaToggleOverview(lua_State*) {
        toggleOverview("");
        return 0;
    }

} // namespace hyprdeck
