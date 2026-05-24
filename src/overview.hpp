#pragma once

#include <plugins/PluginAPI.hpp>

#include <string>

namespace hyprdeck {

    void            closeOverview();
    void            openOverview();

    SDispatchResult toggleOverview(std::string args);
    int             luaToggleOverview(lua_State* state);

} // namespace hyprdeck
