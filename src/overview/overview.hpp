#pragma once

#include "runtime_types.hpp"

#include <plugins/PluginAPI.hpp>

#include <functional>
#include <string>

namespace hyprdeck {

    class COverviewController {
      public:
        std::function<void(eRenderStage)> onRenderStage    = [this](const eRenderStage stage) { handleRenderStage(stage); };
        std::function<void()>             onStateChanged   = [this]() { handleStateChanged(); };
        std::function<void(PHLMONITOR)>   onMonitorRemoved = [this](const PHLMONITOR& monitor) { handleMonitorRemoved(monitor); };

        bool                              active() const;
        bool                              rendering() const;
        MONITORID                         hostMonitorID() const;
        MONITORID                         selectedMonitorID() const;
        PHLMONITOR                        hostMonitor() const;
        PHLMONITOR                        selectedMonitor() const;
        double                            zoom() const;

        void                              setZoom(double value);
        void                              open();
        void                              close(bool instant = false);
        void                              selectMonitor(const PHLMONITOR& monitor);
        void                              cycleMonitor(int direction);
        void                              damageHost() const;

        SDispatchResult                   toggle(std::string args);
        int                               luaToggle(lua_State* state);

      private:
        void          openOn(const PHLMONITOR& monitor);
        void          setPointerLocked(const PHLMONITOR& monitor, bool locked);
        void          handleRenderStage(eRenderStage stage);
        void          handleStateChanged();
        void          handleMonitorRemoved(const PHLMONITOR& monitor);
        void          finishClose(const PHLMONITOR& monitor);

        SSessionState m_session;
    };

} // namespace hyprdeck
