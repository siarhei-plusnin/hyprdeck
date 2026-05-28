#pragma once

#include "runtime_types.hpp"

#include <plugins/PluginAPI.hpp>

#include <string>

namespace hyprdeck {

    class COverviewController {
      public:
        bool      active() const;
        MONITORID monitorID() const;
        PHLMONITOR monitor() const;
        double    zoom() const;

        void setZoom(double value);
        void open();
        void close();
        void onRenderStage(eRenderStage stage);

        SDispatchResult toggle(std::string args);
        int             luaToggle(lua_State* state);

      private:
        SSessionState m_session;
    };

} // namespace hyprdeck
