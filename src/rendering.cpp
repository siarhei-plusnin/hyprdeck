#include "rendering.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "naming.hpp"
#include "overlays.hpp"
#include "shortcuts.hpp"
#include "state.hpp"
#include "ui.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <desktop/state/FocusState.hpp>
#include <desktop/view/LayerSurface.hpp>
#include <desktop/view/Window.hpp>
#include <helpers/Monitor.hpp>
#include <managers/PointerManager.hpp>
#include <render/Renderer.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace hyprdeck {
    namespace {

        struct SPreviewRenderData {
            std::array<std::vector<PHLLS>, 4>                       layers;
            std::array<std::vector<PHLLS>, 4>                       externalLayers;
            std::unordered_map<WORKSPACEID, std::vector<PHLWINDOW>> workspaceWindows;
            std::vector<PHLWINDOW>                                  pinnedWindows;
            std::vector<PHLWINDOW>                                  externalWindows;
            PHLWINDOW                                               focusedWindow;
        };

        SP<Render::ITexture> labelTexture(const std::string& label, const int fontSize = 26, const int weight = 700) {
            return textTexture("overview", label, colors::textPrimary(), fontSize, weight);
        }

        void updateCursorCache() {
            const auto texture = g_pPointerManager->getCurrentCursorTexture();
            if (!texture || !texture->ok())
                return;

            const auto& cursor = g_pPointerManager->currentCursorImage();
            const auto  size   = g_pPointerManager->cursorSizeLogical();

            if (size.x <= 0 || size.y <= 0)
                return;

            auto& renderCache         = state().renderCache;
            renderCache.cursorTexture = texture;
            renderCache.cursorHotspot = cursor.hotspot;
            renderCache.cursorSize    = size;
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

                    addTexture(surface->m_current.texture, previewBox, alpha, mainSurface ? rounding : 0, card);
                },
                nullptr);
        }

        bool layerVisibleOnMonitor(const PHLLS& layer, const PHLMONITOR& monitor) {
            if (!layer || !layer->m_mapped || layer->m_fadingOut)
                return false;

            const auto layerMonitor = layer->m_monitor.lock();
            return layerMonitor && monitor && layerMonitor->m_id == monitor->m_id;
        }

        void renderLayerSurfacePreview(const PHLLS& layer, const SWorkspaceCard& card, const PHLMONITOR& monitor) {
            if (!layerVisibleOnMonitor(layer, monitor))
                return;

            const CBox box{layer->m_realPosition->value(), layer->m_realSize->value()};
            drawSurfaceTree(layer->wlSurface()->resource(), box, card.box, monitor, layer->m_alpha->value(), 0);
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
                if (!layerShouldRenderOverOverview(layer))
                    continue;

                renderLayerSurfacePreview(layer, card, monitor);
            }
        }

        SPreviewRenderData buildPreviewRenderData(const PHLMONITOR& monitor) {
            SPreviewRenderData data;
            const auto         focusState = Desktop::focusState();
            data.focusedWindow            = focusState ? focusState->window() : PHLWINDOW{};

            if (!monitor)
                return data;

            for (size_t i = 0; i < data.layers.size() && i < monitor->m_layerSurfaceLayers.size(); ++i) {
                for (const auto& weakLayer : monitor->m_layerSurfaceLayers[i]) {
                    const auto layer = weakLayer.lock();
                    if (!layerVisibleOnMonitor(layer, monitor))
                        continue;

                    if (layerIsExternalOverlay(layer))
                        data.externalLayers[i].push_back(layer);
                    else
                        data.layers[i].push_back(layer);
                }
            }

            for (const auto& window : g_pCompositor->m_windows) {
                if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden())
                    continue;

                if (!windowBelongsToMonitor(window, monitor))
                    continue;

                if (windowIsExternalOverlay(window))
                    data.externalWindows.push_back(window);
                else if (window->m_pinned)
                    data.pinnedWindows.push_back(window);
                else if (window->m_workspace)
                    data.workspaceWindows[window->m_workspace->m_id].push_back(window);
            }

            return data;
        }

        bool shouldPreviewWindow(const PHLWINDOW& window, const SWorkspaceCard& card, const bool floatingPass) {
            if (!window || !window->m_isMapped || window->m_fadingOut || window->isHidden())
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

        void renderWindowPass(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SPreviewRenderData& data, const bool floatingPass) {
            PHLWINDOW  deferredFocusedWindow;

            const auto renderWindowList = [&](const std::vector<PHLWINDOW>& windows) {
                for (const auto& window : windows) {
                    if (!shouldPreviewWindow(window, card, floatingPass))
                        continue;

                    if (window == data.focusedWindow) {
                        deferredFocusedWindow = window;
                        continue;
                    }

                    renderWindowPreview(window, card, monitor);
                }
            };

            if (card.workspace) {
                if (const auto it = data.workspaceWindows.find(card.workspace->m_id); it != data.workspaceWindows.end())
                    renderWindowList(it->second);
            }

            renderWindowList(data.pinnedWindows);

            if (deferredFocusedWindow)
                renderWindowPreview(deferredFocusedWindow, card, monitor);
        }

        void renderWorkspacePreview(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SPreviewRenderData& data) {
            addRect(card.box, colors::workspacePreviewBackground());

            if (card.special && !card.workspace)
                return;

            renderLayerGroupPreview(data.layers[0], card, monitor);
            renderLayerGroupPreview(data.layers[1], card, monitor);

            if (card.workspace) {
                renderWindowPass(card, monitor, data, false);
                renderWindowPass(card, monitor, data, true);
            }

            renderLayerGroupPreview(data.layers[2], card, monitor);
            renderLayerGroupPreview(data.layers[3], card, monitor);
        }

        void renderCard(const SWorkspaceCard& card, const PHLMONITOR& monitor, const SPreviewRenderData& data) {
            const bool current  = cardIsActive(card, monitor);
            const bool selected = cardIsSelected(card);

            if (card.action != EWorkspaceCardAction::SWITCH) {
                if (selected)
                    addRect(expanded(card.box, 7.0), colors::actionCardSelectionGlow(), 16);

                addRect(expanded(card.box, 3.0), selected ? colors::actionCardSelectedBorder() : colors::actionCardBorder(), 14);
                addRect(card.box, colors::actionCardBackground(), 12);

                const auto texture = labelTexture(card.label, 18, 650);
                if (!texture || !texture->ok())
                    return;

                const CBox labelBox{card.box.x + ((card.box.w - texture->m_size.x) / 2.0), card.box.y + ((card.box.h - texture->m_size.y) / 2.0), texture->m_size.x,
                                    texture->m_size.y};
                addTexture(texture, labelBox, 1.0F);
                return;
            }

            if (selected)
                addRect(expanded(card.box, current ? 11.0 : 7.0), current ? colors::activeCardSelectionGlow() : colors::inactiveCardSelectionGlow());

            addRect(expanded(card.box, current ? 7.0 : 3.0), current ? colors::accent() : colors::inactiveCardBorder());
            renderWorkspacePreview(card, monitor, data);

            if (card.label.empty())
                return;

            const auto texture = labelTexture(card.label);
            if (!texture || !texture->ok())
                return;

            const CBox labelBox{card.box.x + 22.0, card.box.y + 18.0, texture->m_size.x, texture->m_size.y};
            addRect(expanded(labelBox, 8.0), colors::labelBackdrop(), 10);
            addTexture(texture, labelBox, 1.0F);
        }

        void renderEmptyWorkspaceBackground(const PHLMONITOR& monitor, const SPreviewRenderData& data) {
            const auto viewSize = monitor->m_transformedSize;
            addRect(CBox{0, 0, viewSize.x, viewSize.y}, colors::opaqueBlack());

            const SWorkspaceCard backgroundCard{.box = CBox{0, 0, viewSize.x, viewSize.y}};
            renderLayerGroupPreview(data.layers[0], backgroundCard, monitor);
        }

        void renderExternalOverlays(const PHLMONITOR& monitor, const SPreviewRenderData& data) {
            const auto card = monitorCard(monitor);

            renderLayerGroupOnMonitor(data.externalLayers[0], monitor);
            renderLayerGroupOnMonitor(data.externalLayers[1], monitor);

            for (const auto& window : data.externalWindows)
                renderWindowPreview(window, card, monitor);

            renderLayerGroupOnMonitor(data.externalLayers[2], monitor);
            renderLayerGroupOnMonitor(data.externalLayers[3], monitor);
        }

    } // namespace

    void renderOverview(const PHLMONITOR& monitor) {
        recalculateCards(monitor);

        const auto viewSize = monitor->m_transformedSize;
        const auto data     = buildPreviewRenderData(monitor);
        if (!activeWorkspaceBackground())
            renderEmptyWorkspaceBackground(monitor, data);

        addRect(CBox{0, 0, viewSize.x, viewSize.y}, colors::overviewScrim());

        const auto& layout = state().layout;
        for (const auto& card : layout.cards)
            renderCard(card, monitor, data);

        for (const auto& card : layout.specialCards)
            renderCard(card, monitor, data);

        renderShortcutFooter(monitor);
        renderNamingPrompt(monitor);
        renderShortcutMenu(monitor);
        renderExternalOverlays(monitor, data);
    }

    void renderCursorOverlay(const PHLMONITOR& monitor) {
        updateCursorCache();

        g_pHyprRenderer->setCursorHidden(false);

        updateCursorCache();

        const auto& renderCache = state().renderCache;
        const auto  pos         = g_pPointerManager->position() - monitor->m_position - renderCache.cursorHotspot;

        if (!renderCache.cursorTexture || !renderCache.cursorTexture->ok()) {
            const auto pointer = (g_pPointerManager->position() - monitor->m_position) * monitor->m_scale;
            addRect(CBox{pointer.x, pointer.y, 4, 20}, colors::fallbackCursor());
            addRect(CBox{pointer.x, pointer.y, 14, 4}, colors::fallbackCursor());
            return;
        }

        addTexture(renderCache.cursorTexture, CBox{pos * monitor->m_scale, renderCache.cursorSize * monitor->m_scale});
    }

    void clearRenderCache() {
        clearTextTextureCache();
        state().renderCache.cursorTexture.reset();
    }

} // namespace hyprdeck
