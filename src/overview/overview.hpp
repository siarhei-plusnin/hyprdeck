#pragma once

#include "runtime_types.hpp"

#include <plugins/PluginAPI.hpp>

#include <functional>
#include <string>

namespace hyprdeck {

    class COverviewController {
      public:
        std::function<void(eRenderStage)> onRenderStage = [this](const eRenderStage stage) { handleRenderStage(stage); };

        bool      active() const;
        MONITORID monitorID() const;
        PHLMONITOR monitor() const;
        double    zoom() const;

        void setZoom(double value);
        void open();
        void close();

        SDispatchResult toggle(std::string args);
        int             luaToggle(lua_State* state);

      private:
        void handleRenderStage(eRenderStage stage);

        SSessionState m_session;
    };

} // namespace hyprdeck
