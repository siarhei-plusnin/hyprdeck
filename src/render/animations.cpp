#include "animations.hpp"

#include "layout.hpp"
#include "overview.hpp"
#include "plugin.hpp"

#include <config/ConfigValue.hpp>
#include <config/shared/animation/AnimationTree.hpp>
#include <helpers/Monitor.hpp>
#include <managers/animation/AnimationManager.hpp>

#include <cmath>
#include <string>

namespace hyprdeck {
    namespace {

        bool beingAnimated(const PHLANIMVAR<float>& var) {
            return var && var->isBeingAnimated();
        }

        bool pluginAnimationsEnabled() {
            return activePlugin() && activePlugin()->config().animationsEnabled();
        }

        bool hyprlandAnimationsEnabled() {
            static auto enabled = CConfigValue<Config::INTEGER>("animations:enabled");
            return *enabled;
        }

        bool animationsEnabled() {
            return pluginAnimationsEnabled() && hyprlandAnimationsEnabled();
        }

        float sampledFloat(const PHLANIMVAR<float>& var, const float fallback) {
            if (!var)
                return fallback;
            if (!var->isBeingAnimated())
                return var->value();
            if (!animationsEnabled() || !var->enabled())
                return var->goal();

            return var->begun() + ((var->goal() - var->begun()) * var->getCurveValue());
        }

        void finishIfComplete(const PHLANIMVAR<float>& var) {
            if (var && var->isBeingAnimated() && (!animationsEnabled() || !var->enabled() || var->getPercent() >= 1.0F))
                var->warp();
        }

        auto animationConfig(const std::string& leaf) {
            if (!Config::animationTree() || !Config::animationTree()->nodeExists(leaf))
                return SP<Hyprutils::Animation::SAnimationPropertyConfig>{};

            return Config::animationTree()->getAnimationPropertyConfig(leaf);
        }

        bool leafEnabled(const std::string& leaf) {
            const auto config = animationConfig(leaf);
            return animationsEnabled() && config && config->pValues && config->pValues->internalEnabled == 1;
        }

        bool shouldAnimateFloat(const std::string& leaf, const float from, const float to) {
            return leafEnabled(leaf) && std::abs(from - to) > 0.001F;
        }

        void damageAnimationMonitor(const MONITORID monitorID) {
            if (monitorID == MONITOR_INVALID)
                return;

            const auto monitor = activePlugin()->hyprland().monitorFromID(monitorID);
            activePlugin()->hyprland().damageMonitor(monitor);
            activePlugin()->hyprland().scheduleAnimationFrame(monitor);
        }

    } // namespace

    void CAnimationController::reset() {
        m_overviewOpacity.reset();
        m_normalCamera.reset();
        m_specialCamera.reset();
        m_zoom.reset();
        m_specialCardAppearance.reset();
        m_specialCardClose.reset();
        m_specialCardAppearanceID = WORKSPACE_INVALID;
        m_specialCardClosing      = SWorkspaceCard{.id = WORKSPACE_INVALID};
        m_monitorID               = MONITOR_INVALID;
    }

    bool CAnimationController::active() const {
        return pluginAnimationsEnabled() &&
            (overviewAnimating() || beingAnimated(m_normalCamera) || beingAnimated(m_specialCamera) || beingAnimated(m_zoom) || beingAnimated(m_specialCardAppearance) ||
             beingAnimated(m_specialCardClose));
    }

    void CAnimationController::update(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        if (!pluginAnimationsEnabled()) {
            reset();
            return;
        }

        m_monitorID = monitor->m_id;

        if (beingAnimated(m_zoom)) {
            activePlugin()->layout().applyZoom(sampledFloat(m_zoom, static_cast<float>(activePlugin()->overview().zoom())), m_zoomNormalCameraRatio, m_zoomSpecialCameraRatio,
                                               monitor);
            finishIfComplete(m_zoom);
        }

        if (beingAnimated(m_normalCamera)) {
            activePlugin()->layout().setCameraX(sampledFloat(m_normalCamera, static_cast<float>(activePlugin()->layout().cameraX())));
            activePlugin()->layout().invalidate();
            finishIfComplete(m_normalCamera);
        }

        if (beingAnimated(m_specialCamera)) {
            activePlugin()->layout().setSpecialCameraX(sampledFloat(m_specialCamera, static_cast<float>(activePlugin()->layout().specialCameraX())));
            activePlugin()->layout().invalidate();
            finishIfComplete(m_specialCamera);
        }

        finishIfComplete(m_overviewOpacity);
        finishIfComplete(m_specialCardAppearance);
        finishIfComplete(m_specialCardClose);

        if (m_specialCardClosing.id != WORKSPACE_INVALID && m_specialCardClose && !m_specialCardClose->isBeingAnimated() && m_specialCardClose->value() <= 0.001F)
            m_specialCardClosing = SWorkspaceCard{.id = WORKSPACE_INVALID};
    }

    void CAnimationController::ensureOverviewAnimations() {
        if (!g_pAnimationManager)
            return;

        if (!m_overviewOpacity) {
            g_pAnimationManager->createAnimation(1.0F, m_overviewOpacity, animationConfig("layersIn"), AVARDAMAGE_NONE);
            m_overviewOpacity->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
        }
    }

    void CAnimationController::startOverviewOpen(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        if (!pluginAnimationsEnabled()) {
            reset();
            return;
        }

        ensureOverviewAnimations();
        if (!m_overviewOpacity)
            return;

        m_monitorID = monitor->m_id;

        m_overviewOpacity->setConfig(animationConfig("layersIn"));
        if (shouldAnimateFloat("layersIn", 0.0F, 1.0F))
            m_overviewOpacity->setValueAndWarp(0.0F);
        else
            m_overviewOpacity->setValueAndWarp(1.0F);
        *m_overviewOpacity = 1.0F;
    }

    bool CAnimationController::startOverviewClose(const PHLMONITOR& monitor) {
        if (!monitor)
            return false;

        if (!pluginAnimationsEnabled()) {
            reset();
            return false;
        }

        ensureOverviewAnimations();
        if (!m_overviewOpacity)
            return false;

        m_monitorID = monitor->m_id;

        bool animated = false;

        m_overviewOpacity->setConfig(animationConfig("layersOut"));
        if (shouldAnimateFloat("layersOut", m_overviewOpacity->value(), 0.0F)) {
            *m_overviewOpacity = 0.0F;
            animated           = true;
        } else
            m_overviewOpacity->setValueAndWarp(0.0F);

        return animated;
    }

    bool CAnimationController::overviewAnimating() const {
        return pluginAnimationsEnabled() && beingAnimated(m_overviewOpacity);
    }

    float CAnimationController::overviewOpacity() const {
        return sampledFloat(m_overviewOpacity, 1.0F);
    }

    void CAnimationController::ensureSpecialCardAppearanceAnimation() {
        if (m_specialCardAppearance || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(1.0F, m_specialCardAppearance, animationConfig("specialWorkspaceIn"), AVARDAMAGE_NONE);
        m_specialCardAppearance->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
    }

    void CAnimationController::ensureSpecialCardCloseAnimation() {
        if (m_specialCardClose || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(1.0F, m_specialCardClose, animationConfig("specialWorkspaceOut"), AVARDAMAGE_NONE);
        m_specialCardClose->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
    }

    void CAnimationController::startSpecialCardAppearance(const WORKSPACEID id, const PHLMONITOR& monitor) {
        if (!monitor || id == WORKSPACE_INVALID || !shouldAnimateFloat("specialWorkspaceIn", 0.0F, 1.0F))
            return;

        ensureSpecialCardAppearanceAnimation();
        if (!m_specialCardAppearance)
            return;

        m_monitorID               = monitor->m_id;
        m_specialCardAppearanceID = id;
        m_specialCardAppearance->setConfig(animationConfig("specialWorkspaceIn"));
        m_specialCardAppearance->setValueAndWarp(0.0F);
        *m_specialCardAppearance = 1.0F;
    }

    void CAnimationController::startSpecialCardClose(const SWorkspaceCard& card, const PHLMONITOR& monitor) {
        if (!monitor || !card.special || card.id == WORKSPACE_INVALID || !shouldAnimateFloat("specialWorkspaceOut", 1.0F, 0.0F))
            return;

        ensureSpecialCardCloseAnimation();
        if (!m_specialCardClose)
            return;

        m_monitorID          = monitor->m_id;
        m_specialCardClosing = card;
        m_specialCardClose->setConfig(animationConfig("specialWorkspaceOut"));
        m_specialCardClose->setValueAndWarp(1.0F);
        *m_specialCardClose = 0.0F;
    }

    float CAnimationController::specialCardOpacity(const WORKSPACEID id) const {
        float opacity = 1.0F;

        if (id == m_specialCardAppearanceID)
            opacity *= sampledFloat(m_specialCardAppearance, 1.0F);

        if (id == m_specialCardClosing.id)
            opacity *= sampledFloat(m_specialCardClose, 1.0F);

        return opacity;
    }

    bool CAnimationController::closingSpecialCard(SWorkspaceCard& card, float& opacity) const {
        if (m_specialCardClosing.id == WORKSPACE_INVALID)
            return false;

        opacity = sampledFloat(m_specialCardClose, 0.0F);
        if (opacity <= 0.001F)
            return false;

        card = m_specialCardClosing;
        return true;
    }

    void CAnimationController::ensureNormalCameraAnimation() {
        if (m_normalCamera || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(static_cast<float>(activePlugin()->layout().cameraX()), m_normalCamera, animationConfig("workspacesIn"), AVARDAMAGE_NONE);
        m_normalCamera->setUpdateCallback([this](auto) {
            const auto monitor = activePlugin()->hyprland().monitorFromID(m_monitorID);
            if (!monitor)
                return;

            activePlugin()->layout().setCameraX(m_normalCamera->value());
            activePlugin()->layout().invalidate();
            damageAnimationMonitor(m_monitorID);
        });
    }

    void CAnimationController::ensureSpecialCameraAnimation() {
        if (m_specialCamera || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(static_cast<float>(activePlugin()->layout().specialCameraX()), m_specialCamera, animationConfig("specialWorkspaceIn"),
                                             AVARDAMAGE_NONE);
        m_specialCamera->setUpdateCallback([this](auto) {
            const auto monitor = activePlugin()->hyprland().monitorFromID(m_monitorID);
            if (!monitor)
                return;

            activePlugin()->layout().setSpecialCameraX(m_specialCamera->value());
            activePlugin()->layout().invalidate();
            damageAnimationMonitor(m_monitorID);
        });
    }

    void CAnimationController::ensureZoomAnimation() {
        if (m_zoom || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(static_cast<float>(activePlugin()->overview().zoom()), m_zoom, animationConfig("workspacesIn"), AVARDAMAGE_NONE);
        m_zoom->setUpdateCallback([this](auto) {
            const auto monitor = activePlugin()->hyprland().monitorFromID(m_monitorID);
            if (!monitor)
                return;

            activePlugin()->layout().applyZoom(m_zoom->value(), m_zoomNormalCameraRatio, m_zoomSpecialCameraRatio, monitor);
            damageAnimationMonitor(m_monitorID);
        });
    }

    bool CAnimationController::animateNormalCamera(const double from, const double to, const PHLMONITOR& monitor) {
        if (!monitor || !shouldAnimateFloat("workspacesIn", static_cast<float>(from), static_cast<float>(to)))
            return false;

        ensureNormalCameraAnimation();
        if (!m_normalCamera)
            return false;

        m_monitorID = monitor->m_id;
        m_normalCamera->setConfig(animationConfig("workspacesIn"));
        m_normalCamera->setValueAndWarp(static_cast<float>(from));
        *m_normalCamera = static_cast<float>(to);
        return true;
    }

    bool CAnimationController::animateSpecialCamera(const double from, const double to, const PHLMONITOR& monitor) {
        if (!monitor || !shouldAnimateFloat("specialWorkspaceIn", static_cast<float>(from), static_cast<float>(to)))
            return false;

        ensureSpecialCameraAnimation();
        if (!m_specialCamera)
            return false;

        m_monitorID = monitor->m_id;
        m_specialCamera->setConfig(animationConfig("specialWorkspaceIn"));
        m_specialCamera->setValueAndWarp(static_cast<float>(from));
        *m_specialCamera = static_cast<float>(to);
        return true;
    }

    bool CAnimationController::animateZoom(const double from, const double to, const double normalCameraRatio, const double specialCameraRatio, const PHLMONITOR& monitor) {
        if (!monitor || !shouldAnimateFloat("workspacesIn", static_cast<float>(from), static_cast<float>(to)))
            return false;

        ensureZoomAnimation();
        if (!m_zoom)
            return false;

        m_monitorID              = monitor->m_id;
        m_zoomNormalCameraRatio  = normalCameraRatio;
        m_zoomSpecialCameraRatio = specialCameraRatio;
        m_zoom->setConfig(animationConfig("workspacesIn"));
        m_zoom->setValueAndWarp(static_cast<float>(from));
        *m_zoom = static_cast<float>(to);
        return true;
    }

    double CAnimationController::zoomTarget(const double fallback) const {
        if (m_zoom && m_zoom->isBeingAnimated())
            return m_zoom->goal();

        return fallback;
    }

    void CAnimationController::cancelCameraAnimations() {
        cancelNormalCameraAnimation();
        cancelSpecialCameraAnimation();
    }

    void CAnimationController::cancelNormalCameraAnimation() {
        if (m_normalCamera && m_normalCamera->isBeingAnimated())
            m_normalCamera->setValueAndWarp(m_normalCamera->value());
    }

    void CAnimationController::cancelSpecialCameraAnimation() {
        if (m_specialCamera && m_specialCamera->isBeingAnimated())
            m_specialCamera->setValueAndWarp(m_specialCamera->value());
    }

    void CAnimationController::cancelZoomAnimation() {
        if (m_zoom && m_zoom->isBeingAnimated())
            m_zoom->setValueAndWarp(m_zoom->value());
    }

} // namespace hyprdeck
