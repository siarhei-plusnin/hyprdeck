#include "overlays.hpp"

#include "config.hpp"

#include <Compositor.hpp>
#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace hyprdeck {
    namespace {

        std::string lower(std::string_view value) {
            std::string lowered;
            lowered.reserve(value.size());
            for (const char character : value)
                lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));

            return lowered;
        }

        template <size_t SIZE>
        bool namespaceMatches(std::string_view value, const std::array<std::string_view, SIZE>& builtInNames, const std::vector<std::string>& configuredNames) {
            const auto lowered = lower(value);

            if (std::ranges::any_of(builtInNames, [&](const auto name) { return lowered.contains(name); }))
                return true;

            return std::ranges::any_of(configuredNames, [&](const auto& name) { return lowered.contains(name); });
        }

        bool layerReadyForDetection(const PHLLS& layer) {
            return layer && layer->m_mapped && !layer->m_fadingOut;
        }

        bool windowReadyForDetection(const PHLWINDOW& window) {
            return window && window->m_isMapped && !window->m_fadingOut && !window->isHidden();
        }

        bool windowVisibleOnMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor) {
            if (!windowReadyForDetection(window) || !monitor)
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

        bool layerBlocksOverviewInput(const PHLLS& layer) {
            return layer && (layer->m_interactivity != 0 || namespaceMatchesInputOverlay(layer->m_namespace));
        }

        bool windowBlocksOverviewInput(const PHLWINDOW& window) {
            return window && (namespaceMatchesInputOverlay(window->m_class) || namespaceMatchesInputOverlay(window->m_initialClass));
        }

        bool externalOverlayLayerActive(const PHLMONITOR& monitor) {
            if (!monitor)
                return false;

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

} // namespace hyprdeck
