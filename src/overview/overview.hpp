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
        bool      rendering() const;
        MONITORID monitorID() const;
        PHLMONITOR monitor() const;
        double    zoom() const;

        void setZoom(double value);
        void open();
        void close(bool instant = false);

        SDispatchResult toggle(std::string args);
        int             luaToggle(lua_State* state);

      private:
        void handleRenderStage(eRenderStage stage);
        void finishClose(const PHLMONITOR& monitor);

        SSessionState m_session;
    };

} // namespace hyprdeck
