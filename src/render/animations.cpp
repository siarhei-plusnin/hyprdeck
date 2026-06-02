#include "animations.hpp"

#include "layout.hpp"
#include "overview.hpp"
#include "plugin.hpp"

#include <config/ConfigValue.hpp>
#include <config/shared/animation/AnimationTree.hpp>
#include <helpers/Monitor.hpp>
#include <managers/animation/AnimationManager.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace hyprdeck {
    namespace {

        struct SAnimationStyle {
            std::string name;
            std::string direction;
            float       percent    = 1.0F;
            bool        hasPercent = false;
        };

        bool beingAnimated(const PHLANIMVAR<float>& var) {
            return var && var->isBeingAnimated();
        }

        bool beingAnimated(const PHLANIMVAR<Vector2D>& var) {
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

        Vector2D sampledVector(const PHLANIMVAR<Vector2D>& var, const Vector2D fallback) {
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

        void finishIfComplete(const PHLANIMVAR<Vector2D>& var) {
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

        std::vector<std::string> styleTokens(const std::string& rawStyle) {
            std::vector<std::string> tokens;
            std::string              current;

            for (const auto ch : rawStyle) {
                if (std::isspace(static_cast<unsigned char>(ch))) {
                    if (!current.empty()) {
                        tokens.push_back(current);
                        current.clear();
                    }
                    continue;
                }

                current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }

            if (!current.empty())
                tokens.push_back(current);

            return tokens;
        }

        SAnimationStyle parseStyle(const std::string& rawStyle) {
            SAnimationStyle style;
            const auto      tokens = styleTokens(rawStyle);
            if (tokens.empty())
                return style;

            style.name = tokens.front();
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& token = tokens[i];
                if (token.ends_with('%')) {
                    try {
                        style.percent    = std::clamp(std::stof(token.substr(0, token.size() - 1)) / 100.0F, 0.0F, 2.0F);
                        style.hasPercent = true;
                    } catch (...) {
                    }
                    continue;
                }

                if (style.direction.empty())
                    style.direction = token;
            }

            return style;
        }

        SAnimationStyle styleForLeaf(const std::string& leaf) {
            const auto config = animationConfig(leaf);
            if (!config || !config->pValues)
                return {};

            return parseStyle(config->pValues->internalStyle);
        }

        bool styleUsesSlide(const SAnimationStyle& style) {
            return style.name.starts_with("slide") && style.name != "slidefade" && style.name != "slidefadevert";
        }

        bool styleUsesSlideFade(const SAnimationStyle& style) {
            return style.name.starts_with("slidefade");
        }

        bool styleUsesAnySlide(const SAnimationStyle& style) {
            return styleUsesSlide(style) || styleUsesSlideFade(style);
        }

        bool workspaceStyleUsesMotion(const SAnimationStyle& style) {
            return style.name != "fade";
        }

        bool styleUsesPopin(const SAnimationStyle& style) {
            return style.name.starts_with("popin");
        }

        float popinScale(const SAnimationStyle& style) {
            return std::clamp(style.hasPercent ? style.percent : 0.80F, 0.05F, 1.0F);
        }

        Vector2D slideOffset(const SAnimationStyle& style, const PHLMONITOR& monitor, const bool in) {
            if (!monitor)
                return {};

            bool vertical = style.name.starts_with("slidevert") || style.name.starts_with("slidefadevert");
            bool positive = true;

            if (style.direction == "top") {
                vertical = true;
                positive = false;
            } else if (style.direction == "bottom") {
                vertical = true;
                positive = true;
            } else if (style.direction == "left") {
                vertical = false;
                positive = false;
            } else if (style.direction == "right") {
                vertical = false;
                positive = true;
            } else if (style.direction.empty()) {
                vertical = monitor->m_transformedSize.y <= monitor->m_transformedSize.x;
                positive = false;
            }

            const auto  viewSize = monitor->m_transformedSize;
            const float distance = (style.hasPercent ? style.percent : 1.0F) * static_cast<float>(vertical ? viewSize.y : viewSize.x);
            const float sign     = positive ? 1.0F : -1.0F;
            const auto  offset   = vertical ? Vector2D{0.0, distance * sign} : Vector2D{distance * sign, 0.0};

            return in ? offset : offset * -1.0;
        }

        bool shouldAnimateFloat(const std::string& leaf, const float from, const float to) {
            return leafEnabled(leaf) && std::abs(from - to) > 0.001F;
        }

        bool shouldAnimateVector(const std::string& leaf, const Vector2D& from, const Vector2D& to) {
            return leafEnabled(leaf) && from != to;
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
        m_overviewScale.reset();
        m_overviewOffset.reset();
        m_normalCamera.reset();
        m_specialCamera.reset();
        m_zoom.reset();
        m_specialCardAppearance.reset();
        m_specialCardAppearanceID = WORKSPACE_INVALID;
        m_specialCardStartScale   = 1.0F;
        m_monitorID = MONITOR_INVALID;
    }

    bool CAnimationController::active() const {
        return pluginAnimationsEnabled() && (overviewAnimating() || beingAnimated(m_normalCamera) || beingAnimated(m_specialCamera) || beingAnimated(m_zoom) || beingAnimated(m_specialCardAppearance));
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
            activePlugin()->layout().applyZoom(sampledFloat(m_zoom, static_cast<float>(activePlugin()->overview().zoom())), m_zoomNormalCameraRatio, m_zoomSpecialCameraRatio, monitor);
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
        finishIfComplete(m_overviewScale);
        finishIfComplete(m_overviewOffset);
        finishIfComplete(m_specialCardAppearance);
    }

    void CAnimationController::ensureOverviewAnimations() {
        if (!g_pAnimationManager)
            return;

        if (!m_overviewOpacity) {
            g_pAnimationManager->createAnimation(1.0F, m_overviewOpacity, animationConfig("fadeLayersIn"), AVARDAMAGE_NONE);
            m_overviewOpacity->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
        }

        if (!m_overviewScale) {
            g_pAnimationManager->createAnimation(1.0F, m_overviewScale, animationConfig("layersIn"), AVARDAMAGE_NONE);
            m_overviewScale->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
        }

        if (!m_overviewOffset) {
            g_pAnimationManager->createAnimation(Vector2D{}, m_overviewOffset, animationConfig("layersIn"), AVARDAMAGE_NONE);
            m_overviewOffset->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
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
        if (!m_overviewOpacity || !m_overviewScale || !m_overviewOffset)
            return;

        m_monitorID = monitor->m_id;

        const auto style = styleForLeaf("layersIn");

        m_overviewOpacity->setConfig(animationConfig("fadeLayersIn"));
        if (shouldAnimateFloat("fadeLayersIn", 0.0F, 1.0F))
            m_overviewOpacity->setValueAndWarp(0.0F);
        else
            m_overviewOpacity->setValueAndWarp(1.0F);
        *m_overviewOpacity = 1.0F;

        m_overviewScale->setConfig(animationConfig("layersIn"));
        const float startScale = styleUsesPopin(style) && leafEnabled("layersIn") ? popinScale(style) : 1.0F;
        m_overviewScale->setValueAndWarp(startScale);
        *m_overviewScale = 1.0F;

        m_overviewOffset->setConfig(animationConfig("layersIn"));
        const auto startOffset = styleUsesAnySlide(style) && leafEnabled("layersIn") ? slideOffset(style, monitor, true) : Vector2D{};
        m_overviewOffset->setValueAndWarp(startOffset);
        *m_overviewOffset = Vector2D{};
    }

    bool CAnimationController::startOverviewClose(const PHLMONITOR& monitor) {
        if (!monitor)
            return false;

        if (!pluginAnimationsEnabled()) {
            reset();
            return false;
        }

        ensureOverviewAnimations();
        if (!m_overviewOpacity || !m_overviewScale || !m_overviewOffset)
            return false;

        m_monitorID = monitor->m_id;

        const auto style = styleForLeaf("layersOut");

        bool animated = false;

        m_overviewOpacity->setConfig(animationConfig("fadeLayersOut"));
        if (shouldAnimateFloat("fadeLayersOut", m_overviewOpacity->value(), 0.0F)) {
            *m_overviewOpacity = 0.0F;
            animated           = true;
        } else
            m_overviewOpacity->setValueAndWarp(0.0F);

        m_overviewScale->setConfig(animationConfig("layersOut"));
        const float targetScale = styleUsesPopin(style) && leafEnabled("layersOut") ? popinScale(style) : 1.0F;
        if (shouldAnimateFloat("layersOut", m_overviewScale->value(), targetScale)) {
            *m_overviewScale = targetScale;
            animated         = true;
        } else
            m_overviewScale->setValueAndWarp(targetScale);

        m_overviewOffset->setConfig(animationConfig("layersOut"));
        const auto targetOffset = styleUsesAnySlide(style) && leafEnabled("layersOut") ? slideOffset(style, monitor, false) : Vector2D{};
        if (shouldAnimateVector("layersOut", m_overviewOffset->value(), targetOffset)) {
            *m_overviewOffset = targetOffset;
            animated          = true;
        } else
            m_overviewOffset->setValueAndWarp(targetOffset);

        return animated;
    }

    bool CAnimationController::overviewAnimating() const {
        return pluginAnimationsEnabled() && (beingAnimated(m_overviewOpacity) || beingAnimated(m_overviewScale) || beingAnimated(m_overviewOffset));
    }

    float CAnimationController::overviewOpacity() const {
        return sampledFloat(m_overviewOpacity, 1.0F);
    }

    float CAnimationController::overviewScale() const {
        return sampledFloat(m_overviewScale, 1.0F);
    }

    Vector2D CAnimationController::overviewOffset() const {
        return sampledVector(m_overviewOffset, Vector2D{});
    }

    void CAnimationController::ensureSpecialCardAppearanceAnimation() {
        if (m_specialCardAppearance || !g_pAnimationManager)
            return;

        g_pAnimationManager->createAnimation(1.0F, m_specialCardAppearance, animationConfig("specialWorkspaceIn"), AVARDAMAGE_NONE);
        m_specialCardAppearance->setUpdateCallback([this](auto) { damageAnimationMonitor(m_monitorID); });
    }

    void CAnimationController::startSpecialCardAppearance(const WORKSPACEID id, const PHLMONITOR& monitor) {
        if (!monitor || id == WORKSPACE_INVALID || !shouldAnimateFloat("specialWorkspaceIn", 0.0F, 1.0F))
            return;

        ensureSpecialCardAppearanceAnimation();
        if (!m_specialCardAppearance)
            return;

        const auto style = styleForLeaf("specialWorkspaceIn");

        m_monitorID                = monitor->m_id;
        m_specialCardAppearanceID = id;
        m_specialCardStartScale   = styleUsesPopin(style) && leafEnabled("specialWorkspaceIn") ? popinScale(style) : 1.0F;
        m_specialCardAppearance->setConfig(animationConfig("specialWorkspaceIn"));
        m_specialCardAppearance->setValueAndWarp(0.0F);
        *m_specialCardAppearance = 1.0F;
    }

    float CAnimationController::specialCardOpacity(const WORKSPACEID id) const {
        if (id != m_specialCardAppearanceID)
            return 1.0F;

        return sampledFloat(m_specialCardAppearance, 1.0F);
    }

    float CAnimationController::specialCardScale(const WORKSPACEID id) const {
        if (id != m_specialCardAppearanceID)
            return 1.0F;

        const auto progress = sampledFloat(m_specialCardAppearance, 1.0F);
        return m_specialCardStartScale + ((1.0F - m_specialCardStartScale) * progress);
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

        g_pAnimationManager->createAnimation(static_cast<float>(activePlugin()->layout().specialCameraX()), m_specialCamera, animationConfig("specialWorkspaceIn"), AVARDAMAGE_NONE);
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
        if (!monitor || !shouldAnimateFloat("workspacesIn", static_cast<float>(from), static_cast<float>(to)) || !workspaceStyleUsesMotion(styleForLeaf("workspacesIn")))
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

        m_monitorID                = monitor->m_id;
        m_zoomNormalCameraRatio    = normalCameraRatio;
        m_zoomSpecialCameraRatio   = specialCameraRatio;
        m_zoom->setConfig(animationConfig("workspacesIn"));
        m_zoom->setValueAndWarp(static_cast<float>(from));
        *m_zoom = static_cast<float>(to);
        return true;
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
