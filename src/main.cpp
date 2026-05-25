#include <plugins/PluginAPI.hpp>

#include <config/values/ConfigValues.hpp>
#include <config/values/types/StringValue.hpp>
#include <event/EventBus.hpp>
#include <helpers/Color.hpp>

#include "colors.hpp"
#include "confirmation.hpp"
#include "input.hpp"
#include "constants.hpp"
#include "naming.hpp"
#include "overview.hpp"
#include "rendering.hpp"
#include "shortcuts.hpp"
#include "state.hpp"
#include "textinput_repeat.hpp"
#include "workspace_filter.hpp"

#include <string>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace {

    HANDLE s_pluginHandle = nullptr;

} // namespace

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    s_pluginHandle = handle;

    HyprlandAPI::addConfigValueV2(
        s_pluginHandle,
        Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:named_special_workspaces",
                                                                      "Comma-separated special workspace names shown by hyprdeck's named creator", ""));
    HyprlandAPI::addConfigValueV2(
        s_pluginHandle,
        Config::Values::makeConfigValue<Config::Values::CFloatValue>(
            "plugin:hyprdeck:default_zoom", "Default zoom used when opening hyprdeck", static_cast<Config::FLOAT>(hyprdeck::DEFAULT_ZOOM),
            Config::Values::SFloatValueOptions{.min = static_cast<Config::FLOAT>(hyprdeck::MIN_ZOOM), .max = static_cast<Config::FLOAT>(hyprdeck::MAX_ZOOM)}));
    HyprlandAPI::addConfigValueV2(s_pluginHandle,
                                  Config::Values::makeConfigValue<Config::Values::CBoolValue>("plugin:hyprdeck:active_workspace_background",
                                                                                              "Use the active workspace as hyprdeck's background", true));
    HyprlandAPI::addConfigValueV2(
        s_pluginHandle,
        Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:font_family", "Font family used by hyprdeck text", "monospace"));
    HyprlandAPI::addConfigValueV2(s_pluginHandle,
                                   Config::Values::makeConfigValue<Config::Values::CStringValue>("plugin:hyprdeck:shortcuts_footer",
                                                                                                 "Keyboard shortcuts footer visibility: full, hint, or none", "full"));
    HyprlandAPI::addConfigValueV2(s_pluginHandle,
                                  Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                      "plugin:hyprdeck:blocking_overlays", "Comma-separated case-insensitive overlay name substrings that block hyprdeck input", ""));
    HyprlandAPI::addConfigValueV2(s_pluginHandle,
                                  Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                      "plugin:hyprdeck:non_blocking_overlays", "Comma-separated case-insensitive overlay name substrings rendered over hyprdeck without blocking input", ""));
    HyprlandAPI::addConfigValueV2(s_pluginHandle,
                                  Config::Values::makeConfigValue<Config::Values::CStringValue>(
                                      "plugin:hyprdeck:display_capture_overlays", "Comma-separated case-insensitive capture overlay names hidden under hyprdeck", ""));

    HyprlandAPI::addDispatcherV2(s_pluginHandle, "hyprdeck:toggle", hyprdeck::toggleOverview);
    HyprlandAPI::addLuaFunction(s_pluginHandle, "hyprdeck", "toggle", hyprdeck::luaToggleOverview);

    auto& hooks           = hyprdeck::state().hooks;
    hooks.renderHook      = Event::bus()->m_events.render.stage.listen(hyprdeck::onRenderStage);
    hooks.mouseMoveHook   = Event::bus()->m_events.input.mouse.move.listen(hyprdeck::onMouseMove);
    hooks.mouseButtonHook = Event::bus()->m_events.input.mouse.button.listen(hyprdeck::onMouseButton);
    hooks.mouseAxisHook   = Event::bus()->m_events.input.mouse.axis.listen(hyprdeck::onMouseAxis);
    hooks.keyboardHook    = Event::bus()->m_events.input.keyboard.key.listen(hyprdeck::onKeyboard);

    HyprlandAPI::addNotification(s_pluginHandle, "hyprdeck loaded", hyprdeck::colors::accentOpaque(), 2500);

    return PLUGIN_DESCRIPTION_INFO{
        .name        = "hyprdeck",
        .description = "Workspace overview",
        .author      = "siarhei-plusnin",
        .version     = "0.1.0",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprdeck::closeOverview();
    hyprdeck::resetNamingComponent();
    hyprdeck::resetWorkspaceFilterState();
    hyprdeck::resetConfirmationState();
    hyprdeck::resetShortcutState();
    hyprdeck::resetTextInputRepeatComponent();
    hyprdeck::resetHooks();
    hyprdeck::clearRenderCache();

    if (s_pluginHandle)
        HyprlandAPI::removeDispatcher(s_pluginHandle, "hyprdeck:toggle");

    s_pluginHandle = nullptr;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
