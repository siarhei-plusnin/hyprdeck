#include "workspace_preview_renderer.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "workspaces.hpp"

#include <desktop/state/FocusState.hpp>
#include <desktop/Workspace.hpp>
#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Window.hpp>
#include <output/Monitor.hpp>
#include <protocols/core/Compositor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <string>

namespace hyprdeck {
    namespace {

        SP<Render::ITexture> labelTexture(const std::string& label, const int fontSize = 26, const int weight = 700) {
            return activePlugin()->renderServices().textTexture("overview", label, colors::textPrimary(), fontSize, weight);
        }

        CBox previewContentBox(const CBox& card, const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const double scale    = std::min(card.w / std::max(1.0, viewSize.x), card.h / std::max(1.0, viewSize.y));
            const double width    = viewSize.x * scale;
            const double height   = viewSize.y * scale;

            return CBox{card.x + ((card.w - width) / 2.0), card.y + ((card.h - height) / 2.0), width, height};
        }

        CBox scaleLogicalBoxToCard(const CBox& box, const CBox& card, const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const auto   content  = previewContentBox(card, monitor);
            const double scaleX   = content.w / std::max(1.0, viewSize.x);
            const double scaleY   = content.h / std::max(1.0, viewSize.y);

            return CBox{
                content.x + ((box.x - monitor->m_position.x) * monitor->m_scale * scaleX),
                content.y + ((box.y - monitor->m_position.y) * monitor->m_scale * scaleY),
                box.w * monitor->m_scale * scaleX,
                box.h * monitor->m_scale * scaleY,
            };
        }

        Vector2D surfaceLogicalSize(const SP<CWLSurfaceResource>& surface) {
            if (!surface)
                return {};

            if (surface->m_current.size.x > 0 && surface->m_current.size.y > 0)
                return surface->m_current.size;

            const auto scale = std::max(1, surface->m_current.scale);
            return surface->m_current.bufferSize / scale;
        }

        void drawSurfaceTree(const SP<CWLSurfaceResource>& root, const CBox& baseBox, const CBox& card, const PHLMONITOR& monitor, const float alpha, const int rounding) {
            if (!root)
                return;

            root->breadthfirst(
                [&](SP<CWLSurfaceResource> surface, const Vector2D& offset, void*) {
                    if (!surface || !surface->m_current.texture || surface->m_current.size.x < 1 || surface->m_current.size.y < 1)
                        return;

                    const bool     mainSurface = surface == root;
                    const Vector2D size        = mainSurface ? baseBox.size() : surfaceLogicalSize(surface);
                    const CBox     logicalBox{baseBox.pos() + offset, size};
                    const CBox     previewBox = scaleLogicalBoxToCard(logicalBox, card, monitor);

                    if (previewBox.intersection(card).empty())
                        return;

                    activePlugin()->renderServices().addTexture(surface->m_current.texture, previewBox, alpha, mainSurface ? rounding : 0, card);
                },
                nullptr);
        }

        bool layerVisibleOnMonitor(const PHLLS& layer, const PHLMONITOR& monitor) {
            if (!layer || !layer->m_mapped)
                return false;

            const auto layerMonitor = layer->m_monitor.lock();
            return layerMonitor && layerMonitor->m_id == monitor->m_id;
        }

        void renderLayerSurfacePreview(const PHLLS& layer, const SWorkspaceCard& card, const PHLMONITOR& monitor) {
            if (!layerVisibleOnMonitor(layer, monitor))
                return;

            const auto box = layer->geometricBox(Desktop::View::IGeometric::GEOMETRIC_CURRENT);
            drawSurfaceTree(layer->wlSurface()->resource(), box, card.box, monitor, layer->alpha().value(), 0);
        }

        void renderLayerGroupPreview(const std::vector<PHLLS>& layers, const SWorkspaceCard& card, const PHLMONITOR& monitor) {
            for (const auto& layer : layers)
                renderLayerSurfacePreview(layer, card, monitor);
        }

        SWorkspaceCard monitorCard(const PHLMONITOR& monitor) {
            const auto viewSize = monitor->m_transformedSize;
            return SWorkspaceCard{.box = CBox{0, 0, viewSize.x, viewSize.y}};
        }

        void renderLayerGroupOnMonitor(const std::vector<PHLLS>& layers, const PHLMONITOR& monitor) {
            const auto card = monitorCard(monitor);
            for (const auto& layer : layers) {
                if (!activePlugin()->overlays().layerShouldRenderOverOverview(layer))
                    continue;

                renderLayerSurfacePreview(layer, card, monitor);
            }
        }

        bool shouldPreviewWindow(const PHLWINDOW& window, const bool floatingPass) {
            if (!window || !window->m_isMapped || window->isHidden())
                return false;

            return window->m_isFloating == floatingPass;
        }

        void renderWindowPreview(const PHLWINDOW& window, const SWorkspaceCard& card, const PHLMONITOR& monitor) {
            const auto root = window->wlSurface()->resource();
            if (!root)
                return;

            const CBox box      = window->getWindowMainSurfaceBox();
            const int  rounding = window->rounding() * monitor->m_scale * (card.box.w / monitor->m_transformedSize.x);
            drawSurfaceTree(root, box, card.box, monitor, std::clamp(window->effectiveAlpha(), 0.0F, 1.0F), rounding);
        }

        void renderWindowPass(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot, const bool floatingPass) {
            PHLWINDOW  deferredFocusedWindow;
            const auto workspaceFocusedWindow = card.workspace ? card.workspace->getLastFocusedWindow() : PHLWINDOW{};
            const auto focusedWindow          = workspaceFocusedWindow ? workspaceFocusedWindow : snapshot.focusedWindow;

            const auto renderWindowList = [&](const std::vector<PHLWINDOW>& windows) {
                for (const auto& window : windows) {
                    if (!shouldPreviewWindow(window, floatingPass))
                        continue;

                    if (window == focusedWindow) {
                        deferredFocusedWindow = window;
                        continue;
                    }

                    renderWindowPreview(window, card, monitor);
                }
            };

            if (card.workspace) {
                if (const auto it = snapshot.workspaceWindows.find(card.workspace->m_id); it != snapshot.workspaceWindows.end())
                    renderWindowList(it->second);
            }

            renderWindowList(snapshot.pinnedWindows);

            if (deferredFocusedWindow)
                renderWindowPreview(deferredFocusedWindow, card, monitor);
        }

        void renderWorkspacePreview(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) {
            activePlugin()->renderServices().addRect(card.box, colors::workspacePreviewBackground());

            if (card.special && !card.workspace)
                return;

            renderLayerGroupPreview(snapshot.layers[0], card, monitor);
            renderLayerGroupPreview(snapshot.layers[1], card, monitor);

            if (card.workspace) {
                renderWindowPass(card, monitor, snapshot, false);
                renderWindowPass(card, monitor, snapshot, true);
            }

            renderLayerGroupPreview(snapshot.layers[2], card, monitor);
            renderLayerGroupPreview(snapshot.layers[3], card, monitor);
        }

    } // namespace

    SWorkspacePreviewSnapshot CWorkspacePreviewRenderer::buildSnapshot(const PHLMONITOR& monitor) const {
        SWorkspacePreviewSnapshot snapshot;
        const auto                focusState = Desktop::focusState();
        snapshot.focusedWindow               = focusState ? focusState->window() : PHLWINDOW{};

        for (size_t i = 0; i < snapshot.layers.size() && i < monitor->m_layerSurfaceLayers.size(); ++i) {
            for (const auto& weakLayer : monitor->m_layerSurfaceLayers[i]) {
                const auto layer = weakLayer.lock();
                if (!layerVisibleOnMonitor(layer, monitor))
                    continue;

                if (activePlugin()->overlays().layerIsExternalOverlay(layer))
                    snapshot.externalLayers[i].push_back(layer);
                else
                    snapshot.layers[i].push_back(layer);
            }
        }

        for (const auto& window : activePlugin()->hyprland().windows()) {
            if (!window || !window->m_isMapped || window->isHidden())
                continue;

            if (!activePlugin()->workspaces().windowBelongsToMonitor(window, monitor))
                continue;

            if (activePlugin()->overlays().windowIsExternalOverlay(window))
                snapshot.externalWindows.push_back(window);
            else if (window->m_pinned)
                snapshot.pinnedWindows.push_back(window);
            else if (window->m_workspace)
                snapshot.workspaceWindows[window->m_workspace->m_id].push_back(window);
        }

        return snapshot;
    }

    void CWorkspacePreviewRenderer::renderCard(const SWorkspaceCard& card, const PHLMONITOR& monitor, const PHLMONITOR& selectedMonitor,
                                               const SWorkspacePreviewSnapshot& snapshot) const {
        const bool current  = activePlugin()->workspaces().cardIsActive(card, monitor);
        const bool selected = activePlugin()->workspaces().cardIsSelected(card);

        if (selected)
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(card.box, current ? 11.0 : 7.0),
                                                     current ? colors::activeCardSelectionGlow() : colors::inactiveCardSelectionGlow());

        const auto activeColor = card.sourceOutputName.empty() ? colors::accent() : activePlugin()->config().outputColor(card.sourceOutputName);
        activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(card.box, current ? 7.0 : 3.0), current ? activeColor : colors::inactiveCardBorder());
        renderWorkspacePreview(card, monitor, snapshot);

        if (card.label.empty())
            return;

        const auto texture      = labelTexture(card.label);
        const bool showOwner    = !card.special && card.workspace && selectedMonitor && card.sourceMonitorID != selectedMonitor->m_id && !card.sourceOutputName.empty();
        const auto ownerTexture = showOwner ? labelTexture(card.sourceOutputName, 16, 600) : SP<Render::ITexture>{};
        if (!texture || !texture->ok() || (showOwner && (!ownerTexture || !ownerTexture->ok())))
            return;

        const double ownerGap = showOwner ? 4.0 : 0.0;
        const double contentW = showOwner ? std::max(texture->m_size.x, ownerTexture->m_size.x) : texture->m_size.x;
        const double labelW   = std::min(contentW, std::max(1.0, card.box.w - 44.0));
        const double labelH   = texture->m_size.y + ownerGap + (showOwner ? ownerTexture->m_size.y : 0.0);
        const CBox   labelBox{card.box.x + 22.0, card.box.y + 18.0, labelW, labelH};
        activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(labelBox, 8.0), colors::labelBackdrop(), 10);
        activePlugin()->renderServices().addTexture(texture, CBox{labelBox.x, labelBox.y, texture->m_size.x, texture->m_size.y}, 1.0F, 0, labelBox);
        if (showOwner)
            activePlugin()->renderServices().addTexture(ownerTexture, CBox{labelBox.x, labelBox.y + texture->m_size.y + ownerGap, ownerTexture->m_size.x, ownerTexture->m_size.y},
                                                        1.0F, 0, labelBox);
    }

    void CWorkspacePreviewRenderer::renderEmptyWorkspaceBackground(const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) const {
        const auto viewSize = monitor->m_transformedSize;
        activePlugin()->renderServices().addRect(CBox{0, 0, viewSize.x, viewSize.y}, colors::opaqueBlack());

        const SWorkspaceCard backgroundCard{.box = CBox{0, 0, viewSize.x, viewSize.y}};
        renderLayerGroupPreview(snapshot.layers[0], backgroundCard, monitor);
    }

    void CWorkspacePreviewRenderer::renderExternalOverlays(const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) const {
        const auto card = monitorCard(monitor);

        renderLayerGroupOnMonitor(snapshot.externalLayers[0], monitor);
        renderLayerGroupOnMonitor(snapshot.externalLayers[1], monitor);

        for (const auto& window : snapshot.externalWindows)
            renderWindowPreview(window, card, monitor);

        renderLayerGroupOnMonitor(snapshot.externalLayers[2], monitor);
        renderLayerGroupOnMonitor(snapshot.externalLayers[3], monitor);
    }

} // namespace hyprdeck
