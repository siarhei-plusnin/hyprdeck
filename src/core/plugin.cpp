#include "plugin.hpp"

#include "colors.hpp"
#include "confirmation.hpp"
#include "constants.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "rendering.hpp"
#include "shortcuts.hpp"
#include "textinput_repeat.hpp"
#include "workspace_filter.hpp"

#include <config/values/ConfigValues.hpp>
#include <config/values/types/StringValue.hpp>

#include <memory>
#include <utility>

extern "C" {
#include <lua.h>
}

namespace hyprdeck {
    namespace {

        std::unique_ptr<CHyprdeckPlugin> g_plugin;

        SDispatchResult                  dispatchToggle(std::string args) {
            if (!g_plugin)
                return SDispatchResult{.passEvent = false, .success = false};

            return g_plugin->toggle(std::move(args));
        }

        int luaDispatchToggle(lua_State* state) {
            if (!g_plugin)
                return 0;

            return g_plugin->luaToggle(state);
        }

    } // namespace

    CHyprdeckPlugin::CHyprdeckPlugin(HANDLE handle) : m_handle(handle) {}

    void CHyprdeckPlugin::registerConfig() {
        HyprlandAPI::addConfigValueV2(m_handle,
                                      Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                          "plugin:hyprdeck:named_special_workspaces", "Comma-separated special workspace names shown by hyprdeck's named creator", ""));
        HyprlandAPI::addConfigValueV2(m_handle,
                                      Config::Values::makeConfigValue<Config::Values::CFloatValue>(
                                          "plugin:hyprdeck:default_zoom", "Default zoom used when opening hyprdeck", static_cast<Config::FLOAT>(DEFAULT_ZOOM),
                                          Config::Values::SFloatValueOptions{.min = static_cast<Config::FLOAT>(MIN_ZOOM), .max = static_cast<Config::FLOAT>(MAX_ZOOM)}));
        HyprlandAPI::addConfigValueV2(m_handle,
                                      Config::Values::makeConfigValue<Config::Values::CBoolValue>("plugin:hyprdeck:animations", "Enable hyprdeck overview animations", true));
        HyprlandAPI::addConfigValueV2(
            m_handle,
            Config::Values::makeConfigValue<Config::Values::CBoolValue>("plugin:hyprdeck:active_workspace_background", "Use the active workspace as hyprdeck's background", true));
        HyprlandAPI::addConfigValueV2(
            m_handle, Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:font_family", "Font family used by hyprdeck text", "monospace"));
        HyprlandAPI::addConfigValueV2(
            m_handle,
            Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:shortcuts_footer", "Keyboard shortcuts footer visibility: full, hint, or none", "full"));
        HyprlandAPI::addConfigValueV2(
            m_handle, Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:output_colors", "Comma-separated output-name to #RRGGBB color mappings", ""));
        HyprlandAPI::addConfigValueV2(m_handle,
                                      Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                          "plugin:hyprdeck:blocking_overlays", "Comma-separated case-insensitive overlay name substrings that block hyprdeck input", ""));
        HyprlandAPI::addConfigValueV2(
            m_handle,
            Config::Values::makeConfigValue<Config::Values::CStringValue>(
                "plugin:hyprdeck:non_blocking_overlays", "Comma-separated case-insensitive overlay name substrings rendered over hyprdeck without blocking input", ""));
        HyprlandAPI::addConfigValueV2(m_handle,
                                      Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                          "plugin:hyprdeck:display_capture_overlays", "Comma-separated case-insensitive capture overlay names hidden under hyprdeck", ""));
    }

    void CHyprdeckPlugin::init() {
        registerConfig();

        HyprlandAPI::addDispatcherV2(m_handle, "hyprdeck:toggle", dispatchToggle);
        HyprlandAPI::addLuaFunction(m_handle, "hyprdeck", "toggle", luaDispatchToggle);
        m_hooks.registerHooks(m_overview.onRenderStage, m_inputRouter.onMouseMove, m_inputRouter.onMouseButton, m_inputRouter.onMouseAxis, m_inputRouter.onKeyboard,
                              m_overview.onStateChanged, m_overview.onMonitorRemoved);

        HyprlandAPI::addNotification(m_handle, "hyprdeck loaded", colors::accentOpaque(), 2500);
    }

    void CHyprdeckPlugin::shutdown() {
        m_overview.close(true);
        m_naming.resetComponent();
        m_workspaceFilter.resetState();
        m_confirmation.resetState();
        m_shortcuts.resetState();
        m_textInputRepeater.reset();
        m_animations.reset();
        m_hooks.reset();
        m_renderServices.clearTextTextureCache();
        m_renderServices.clearCursorCache();

        if (m_handle)
            HyprlandAPI::removeDispatcher(m_handle, "hyprdeck:toggle");
    }

    SDispatchResult CHyprdeckPlugin::toggle(std::string args) {
        return m_overview.toggle(std::move(args));
    }

    int CHyprdeckPlugin::luaToggle(lua_State* state) {
        return m_overview.luaToggle(state);
    }

    COverviewController& CHyprdeckPlugin::overview() {
        return m_overview;
    }

    CNamingController& CHyprdeckPlugin::naming() {
        return m_naming;
    }

    CWorkspaceFilterController& CHyprdeckPlugin::workspaceFilter() {
        return m_workspaceFilter;
    }

    CConfirmationController& CHyprdeckPlugin::confirmation() {
        return m_confirmation;
    }

    CShortcutMenuController& CHyprdeckPlugin::shortcuts() {
        return m_shortcuts;
    }

    CWorkspaceLayoutController& CHyprdeckPlugin::layout() {
        return m_layout;
    }

    CSelectionController& CHyprdeckPlugin::selection() {
        return m_selection;
    }

    COverviewPointerController& CHyprdeckPlugin::overviewPointer() {
        return m_overviewPointer;
    }

    COverviewKeyboardController& CHyprdeckPlugin::overviewKeyboard() {
        return m_overviewKeyboard;
    }

    CInputRouter& CHyprdeckPlugin::inputRouter() {
        return m_inputRouter;
    }

    CMonitorSelector& CHyprdeckPlugin::monitorSelector() {
        return m_monitorSelector;
    }

    CConfigStore& CHyprdeckPlugin::config() {
        return m_config;
    }

    COverlayPolicy& CHyprdeckPlugin::overlays() {
        return m_overlays;
    }

    CWorkspaceNavigator& CHyprdeckPlugin::navigator() {
        return m_navigator;
    }

    CWorkspaceRepository& CHyprdeckPlugin::workspaces() {
        return m_workspaces;
    }

    CWorkspaceFilterMatcher& CHyprdeckPlugin::workspaceFilterMatcher() {
        return m_workspaceFilterMatcher;
    }

    CWorkspacePreviewRenderer& CHyprdeckPlugin::workspacePreviewRenderer() {
        return m_workspacePreviewRenderer;
    }

    CAnimationController& CHyprdeckPlugin::animations() {
        return m_animations;
    }

    COverviewRenderer& CHyprdeckPlugin::renderer() {
        return m_renderer;
    }

    CShortcutCatalog& CHyprdeckPlugin::shortcutCatalog() {
        return m_shortcutCatalog;
    }

    CRenderServices& CHyprdeckPlugin::renderServices() {
        return m_renderServices;
    }

    CTextInputRepeater& CHyprdeckPlugin::textInputRepeater() {
        return m_textInputRepeater;
    }

    CHyprlandFacade& CHyprdeckPlugin::hyprland() {
        return m_hyprland;
    }

    CHyprdeckPlugin* activePlugin() {
        return g_plugin.get();
    }

    void createPlugin(HANDLE handle) {
        g_plugin = std::make_unique<CHyprdeckPlugin>(handle);
    }

    void destroyPlugin() {
        if (g_plugin)
            g_plugin->shutdown();

        g_plugin.reset();
    }

} // namespace hyprdeck
