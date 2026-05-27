#include "overlays.hpp"

#include "config.hpp"
#include "strings.hpp"

#include <Compositor.hpp>
#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>
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
            return layer && layer->m_mapped && !layer->m_fadingOut;
        }

        bool windowReadyForDetection(const PHLWINDOW& window) {
            return window && window->m_isMapped && !window->m_fadingOut && !window->isHidden();
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
            return namespaceMatches(value, BUILT_IN_NAMES, configuredBlockingOverlayNames());
        }

        bool namespaceMatchesNotificationOverlay(std::string_view value) {
            static constexpr std::array<std::string_view, 8> BUILT_IN_NAMES = {"dunst", "mako", "swaync", "fnott", "notification", "notify", "toast", "wired"};
            return namespaceMatches(value, BUILT_IN_NAMES, configuredNonBlockingOverlayNames());
        }

        bool namespaceMatchesHiddenCaptureOverlay(std::string_view value) {
            static constexpr std::array<std::string_view, 2> BUILT_IN_NAMES = {"still", "grim"};
            return namespaceMatches(value, BUILT_IN_NAMES, configuredDisplayCaptureOverlayNames());
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
            if (!g_pInputManager)
                return false;

            return box.containsPoint(g_pInputManager->getMouseCoordsInternal());
        }

        bool pointerOverLayer(const PHLLS& layer, const PHLMONITOR& monitor) {
            if (!layerVisibleOnMonitor(layer, monitor))
                return false;

            return boxContainsPointer(CBox{layer->m_realPosition->value(), layer->m_realSize->value()});
        }

        bool pointerOverWindow(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!windowVisibleOnMonitor(window, monitor))
                return false;

            return boxContainsPointer(window->getWindowMainSurfaceBox());
        }

    } // namespace

    bool layerIsExternalOverlay(const PHLLS& layer) {
        return layer && (layer->m_interactivity != 0 || namespaceMatchesInputOverlay(layer->m_namespace) || namespaceMatchesNotificationOverlay(layer->m_namespace) ||
                         namespaceMatchesHiddenCaptureOverlay(layer->m_namespace));
    }

    bool layerShouldRenderOverOverview(const PHLLS& layer) {
        if (!layer || !layerIsExternalOverlay(layer))
            return false;

        return !namespaceMatchesHiddenCaptureOverlay(layer->m_namespace);
    }

    bool windowIsExternalOverlay(const PHLWINDOW& window) {
        return window && (namespaceMatchesInputOverlay(window->m_class) || namespaceMatchesInputOverlay(window->m_initialClass) ||
                          namespaceMatchesNotificationOverlay(window->m_class) || namespaceMatchesNotificationOverlay(window->m_initialClass));
    }

    bool externalOverlayActive(const PHLMONITOR& monitor) {
        if (externalOverlayLayerActive(monitor))
            return true;

        for (const auto& window : g_pCompositor->m_windows) {
            if (windowVisibleOnMonitor(window, monitor) && windowBlocksOverviewInput(window))
                return true;
        }

        return false;
    }

    bool pointerOverNotificationOverlay(const PHLMONITOR& monitor) {
        for (const auto& layerRefs : monitor->m_layerSurfaceLayers) {
            for (const auto& layerRef : layerRefs) {
                const auto layer = layerRef.lock();
                if (layerIsNotificationOverlay(layer) && pointerOverLayer(layer, monitor))
                    return true;
            }
        }

        for (const auto& window : g_pCompositor->m_windows) {
            if (windowIsNotificationOverlay(window) && pointerOverWindow(window, monitor))
                return true;
        }

        return false;
    }

} // namespace hyprdeck
