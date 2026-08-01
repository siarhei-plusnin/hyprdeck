#pragma once

#include "hyprland_facade.hpp"
#include "hook_registry.hpp"
#include "input.hpp"
#include "monitor_selector.hpp"
#include "confirmation.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "navigation.hpp"
#include "overview.hpp"
#include "overview_controller.hpp"
#include "overview_interaction.hpp"
#include "overlays.hpp"
#include "animations.hpp"
#include "render_services.hpp"
#include "rendering.hpp"
#include "selection.hpp"
#include "shortcut_catalog.hpp"
#include "shortcuts.hpp"
#include "textinput_repeat.hpp"
#include "workspace_filter.hpp"
#include "workspace_filter_match.hpp"
#include "workspace_preview_renderer.hpp"
#include "workspaces.hpp"

#include <plugins/PluginAPI.hpp>

#include <string>

struct lua_State;

namespace hyprdeck {

    class CHyprdeckPlugin {
      public:
        explicit CHyprdeckPlugin(HANDLE handle);

        void                         init();
        void                         shutdown();

        SDispatchResult              toggle(std::string args);
        SDispatchResult              focusSpecial(std::string args);
        int                          luaToggle(lua_State* state);
        int                          luaFocusSpecial(lua_State* state);

        COverviewController&         overview();
        CNamingController&           naming();
        CWorkspaceFilterController&  workspaceFilter();
        CConfirmationController&     confirmation();
        CShortcutMenuController&     shortcuts();
        CWorkspaceLayoutController&  layout();
        CSelectionController&        selection();
        COverviewPointerController&  overviewPointer();
        COverviewKeyboardController& overviewKeyboard();
        CInputRouter&                inputRouter();
        CMonitorSelector&            monitorSelector();
        CConfigStore&                config();
        COverlayPolicy&              overlays();
        CWorkspaceNavigator&         navigator();
        CWorkspaceRepository&        workspaces();
        CWorkspaceFilterMatcher&     workspaceFilterMatcher();
        CWorkspacePreviewRenderer&   workspacePreviewRenderer();
        CAnimationController&        animations();
        COverviewRenderer&           renderer();
        CShortcutCatalog&            shortcutCatalog();
        CRenderServices&             renderServices();
        CTextInputRepeater&          textInputRepeater();
        CHyprlandFacade&             hyprland();

      private:
        void                        registerConfig();

        HANDLE                      m_handle = nullptr;
        CHyprlandFacade             m_hyprland;
        CTextInputRepeater          m_textInputRepeater;
        CRenderServices             m_renderServices;
        CConfigStore                m_config;
        COverlayPolicy              m_overlays;
        CWorkspaceNavigator         m_navigator;
        CWorkspaceRepository        m_workspaces;
        CWorkspaceFilterMatcher     m_workspaceFilterMatcher;
        CWorkspacePreviewRenderer   m_workspacePreviewRenderer;
        CAnimationController        m_animations;
        COverviewRenderer           m_renderer;
        CShortcutCatalog            m_shortcutCatalog;
        COverviewController         m_overview;
        CNamingController           m_naming;
        CWorkspaceFilterController  m_workspaceFilter;
        CConfirmationController     m_confirmation;
        CShortcutMenuController     m_shortcuts;
        CWorkspaceLayoutController  m_layout;
        CSelectionController        m_selection;
        COverviewPointerController  m_overviewPointer;
        COverviewKeyboardController m_overviewKeyboard;
        CInputRouter                m_inputRouter;
        CMonitorSelector            m_monitorSelector;
        CHookRegistry               m_hooks;
    };

    CHyprdeckPlugin* activePlugin();
    void             createPlugin(HANDLE handle);
    void             destroyPlugin();

} // namespace hyprdeck
