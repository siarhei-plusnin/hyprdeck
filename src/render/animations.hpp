#pragma once

#include "runtime_types.hpp"

#include <helpers/AnimatedVariable.hpp>

namespace hyprdeck {

    class CAnimationController {
      public:
        void reset();

        bool active() const;
        void update(const PHLMONITOR& monitor);

        void startOverviewOpen(const PHLMONITOR& monitor);
        bool startOverviewClose(const PHLMONITOR& monitor);
        bool overviewAnimating() const;

        float overviewOpacity() const;

        void  startSpecialCardAppearance(WORKSPACEID id, const PHLMONITOR& monitor);
        void  startSpecialCardClose(const SWorkspaceCard& card, const PHLMONITOR& monitor);
        float specialCardOpacity(WORKSPACEID id) const;
        bool  closingSpecialCard(SWorkspaceCard& card, float& opacity) const;

        bool animateNormalCamera(double from, double to, const PHLMONITOR& monitor);
        bool animateSpecialCamera(double from, double to, const PHLMONITOR& monitor);
        bool animateZoom(double from, double to, double normalCameraRatio, double specialCameraRatio, const PHLMONITOR& monitor);

        void cancelCameraAnimations();
        void cancelNormalCameraAnimation();
        void cancelSpecialCameraAnimation();
        void cancelZoomAnimation();

      private:
        void ensureOverviewAnimations();
        void ensureNormalCameraAnimation();
        void ensureSpecialCameraAnimation();
        void ensureZoomAnimation();
        void ensureSpecialCardAppearanceAnimation();
        void ensureSpecialCardCloseAnimation();

        MONITORID m_monitorID = MONITOR_INVALID;

        PHLANIMVAR<float> m_overviewOpacity;
        PHLANIMVAR<float> m_normalCamera;
        PHLANIMVAR<float> m_specialCamera;
        PHLANIMVAR<float> m_zoom;
        PHLANIMVAR<float> m_specialCardAppearance;
        PHLANIMVAR<float> m_specialCardClose;
        WORKSPACEID       m_specialCardAppearanceID = WORKSPACE_INVALID;
        SWorkspaceCard    m_specialCardClosing      = SWorkspaceCard{.id = WORKSPACE_INVALID};

        double m_zoomNormalCameraRatio  = 0.0;
        double m_zoomSpecialCameraRatio = 0.0;
    };

} // namespace hyprdeck
