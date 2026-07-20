#include "overlays.hpp"

#include "config.hpp"
#include "plugin.hpp"
#include "strings.hpp"

#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Window.hpp>
#include <output/Monitor.hpp>
#include <managers/input/InputManager.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace hyprdeck {
    namespace {

        template <size_t SIZE>
        bool namespaceMatches(std::string_view value, const std::array<std::string_view, SIZE>& builtInNames, const std::vector<std::string>& configuredNames) {
            return strings::containsAnyLowered(value, builtInNames, configuredNames);
        }

        bool layerReadyForDetection(const PHLLS& layer) {
            return layer && layer->m_mapped;
        }

        bool windowReadyForDetection(const PHLWINDOW& window) {
            return window && window->m_isMapped && !window->isHidden();
        }

        bool layerVisibleOnMonitor(const PHLLS& layer, const PHLMONITOR& monitor) {
            if (!layerReadyForDetection(layer))
                return false;

            const auto layerMonitor = layer->m_monitor.lock();
            return layerMonitor && layerMonitor->m_id == monitor->m_id;
        }

        bool windowVisibleOnMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!windowReadyForDetection(window))
                return false;

            const auto windowMonitor = window->m_monitor.lock();
            return windowMonitor && windowMonitor->m_id == monitor->m_id;
        }

        bool namespaceMatchesInputOverlay(std::string_view value) {
            static constexpr std::array<std::string_view, 10> BUILT_IN_NAMES = {"rofi", "slurp", "selection", "wofi", "fuzzel", "bemenu", "tofi", "walker", "anyrun", "sherlock"};
            return namespaceMatches(value, BUILT_IN_NAMES, activePlugin()->config().blockingOverlayNames());
        }

        bool namespaceMatchesNotificationOverlay(std::string_view value) {
            static constexpr std::array<std::string_view, 8> BUILT_IN_NAMES = {"dunst", "mako", "swaync", "fnott", "notification", "notify", "toast", "wired"};
            return namespaceMatches(value, BUILT_IN_NAMES, activePlugin()->config().nonBlockingOverlayNames());
        }

        bool namespaceMatchesHiddenCaptureOverlay(std::string_view value) {
            static constexpr std::array<std::string_view, 2> BUILT_IN_NAMES = {"still", "grim"};
            return namespaceMatches(value, BUILT_IN_NAMES, activePlugin()->config().displayCaptureOverlayNames());
        }

        bool layerIsNotificationOverlay(const PHLLS& layer) {
            return layer && namespaceMatchesNotificationOverlay(layer->m_namespace);
        }

        bool windowIsNotificationOverlay(const PHLWINDOW& window) {
            return window && (namespaceMatchesNotificationOverlay(window->m_class) || namespaceMatchesNotificationOverlay(window->m_initialClass));
        }

        bool layerBlocksOverviewInput(const PHLLS& layer) {
            return layer && (namespaceMatchesInputOverlay(layer->m_namespace) || (layer->m_interactivity != 0 && !layerIsNotificationOverlay(layer)));
        }

        bool windowBlocksOverviewInput(const PHLWINDOW& window) {
            return window && (namespaceMatchesInputOverlay(window->m_class) || namespaceMatchesInputOverlay(window->m_initialClass));
        }

        bool externalOverlayLayerActive(const PHLMONITOR& monitor) {
            for (const auto& layerRefs : monitor->m_layerSurfaceLayers) {
                for (const auto& layerRef : layerRefs) {
                    const auto layer = layerRef.lock();
                    if (!layerReadyForDetection(layer))
                        continue;

                    if (layerBlocksOverviewInput(layer))
                        return true;
                }
            }

            return false;
        }

        bool boxContainsPointer(const CBox& box) {
            return box.containsPoint(activePlugin()->hyprland().mouseCoords());
        }

        bool pointerOverLayer(const PHLLS& layer, const PHLMONITOR& monitor) {
            if (!layerVisibleOnMonitor(layer, monitor))
                return false;

            return boxContainsPointer(layer->geometricBox(Desktop::View::IGeometric::GEOMETRIC_CURRENT));
        }

        bool pointerOverWindow(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!windowVisibleOnMonitor(window, monitor))
                return false;

            return boxContainsPointer(window->getWindowMainSurfaceBox());
        }

    } // namespace

    bool COverlayPolicy::layerIsExternalOverlay(const PHLLS& layer) const {
        return layer &&
            (layer->m_interactivity != 0 || namespaceMatchesInputOverlay(layer->m_namespace) || namespaceMatchesNotificationOverlay(layer->m_namespace) ||
             namespaceMatchesHiddenCaptureOverlay(layer->m_namespace));
    }

    bool COverlayPolicy::layerShouldRenderOverOverview(const PHLLS& layer) const {
        if (!layer || !layerIsExternalOverlay(layer))
            return false;

        return !namespaceMatchesHiddenCaptureOverlay(layer->m_namespace);
    }

    bool COverlayPolicy::windowIsExternalOverlay(const PHLWINDOW& window) const {
        return window &&
            (namespaceMatchesInputOverlay(window->m_class) || namespaceMatchesInputOverlay(window->m_initialClass) || namespaceMatchesNotificationOverlay(window->m_class) ||
             namespaceMatchesNotificationOverlay(window->m_initialClass));
    }

    bool COverlayPolicy::externalOverlayActive(const PHLMONITOR& monitor) const {
        if (externalOverlayLayerActive(monitor))
            return true;

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (windowVisibleOnMonitor(window, monitor) && windowBlocksOverviewInput(window))
                return true;
        }

        return false;
    }

    bool COverlayPolicy::pointerOverNotificationOverlay(const PHLMONITOR& monitor) const {
        for (const auto& layerRefs : monitor->m_layerSurfaceLayers) {
            for (const auto& layerRef : layerRefs) {
                const auto layer = layerRef.lock();
                if (layerIsNotificationOverlay(layer) && pointerOverLayer(layer, monitor))
                    return true;
            }
        }

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (windowIsNotificationOverlay(window) && pointerOverWindow(window, monitor))
                return true;
        }

        return false;
    }

} // namespace hyprdeck
