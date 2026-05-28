#include <plugins/PluginAPI.hpp>

#include "plugin.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    hyprdeck::createPlugin(handle);
    hyprdeck::activePlugin()->init();

    return PLUGIN_DESCRIPTION_INFO{
        .name        = "hyprdeck",
        .description = "Workspace overview",
        .author      = "siarhei-plusnin",
        .version     = "0.1.0",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
    hyprdeck::destroyPlugin();
}
