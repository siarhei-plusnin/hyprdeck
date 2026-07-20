#include "workspace_preview_renderer.hpp"

#include "colors.hpp"
#include "layout.hpp"
#include "overlays.hpp"
#include "plugin.hpp"
#include "workspaces.hpp"

#include <desktop/state/FocusState.hpp>
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

        CBox scaleLogicalBoxToCard(const CBox& box, const CBox& card, const PHLMONITOR& monitor) {
            const auto   viewSize = monitor->m_transformedSize;
            const double scaleX   = card.w / std::max(1.0, viewSize.x);
            const double scaleY   = card.h / std::max(1.0, viewSize.y);

            return CBox{
                card.x + ((box.x - monitor->m_position.x) * monitor->m_scale * scaleX),
                card.y + ((box.y - monitor->m_position.y) * monitor->m_scale * scaleY),
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

            const auto renderWindowList = [&](const std::vector<PHLWINDOW>& windows) {
                for (const auto& window : windows) {
                    if (!shouldPreviewWindow(window, floatingPass))
                        continue;

                    if (window == snapshot.focusedWindow) {
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

    void CWorkspacePreviewRenderer::renderCard(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SWorkspacePreviewSnapshot& snapshot) const {
        const bool current  = activePlugin()->workspaces().cardIsActive(card, monitor);
        const bool selected = activePlugin()->workspaces().cardIsSelected(card);

        if (selected)
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(card.box, current ? 11.0 : 7.0),
                                                     current ? colors::activeCardSelectionGlow() : colors::inactiveCardSelectionGlow());

        activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(card.box, current ? 7.0 : 3.0),
                                                 current ? colors::accent() : colors::inactiveCardBorder());
        renderWorkspacePreview(card, monitor, snapshot);

        if (card.label.empty())
            return;

        const auto texture = labelTexture(card.label);
        if (!texture || !texture->ok())
            return;

        const CBox labelBox{card.box.x + 22.0, card.box.y + 18.0, texture->m_size.x, texture->m_size.y};
        activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(labelBox, 8.0), colors::labelBackdrop(), 10);
        activePlugin()->renderServices().addTexture(texture, labelBox, 1.0F);
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
