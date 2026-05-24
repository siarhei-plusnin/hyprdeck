#include "layout.hpp"

#include "constants.hpp"
#include "workspaces.hpp"

#include <Compositor.hpp>
#include <desktop/Workspace.hpp>
#include <helpers/Monitor.hpp>
#include <managers/input/InputManager.hpp>
#include <render/Renderer.hpp>

#include <algorithm>
#include <optional>
#include <string>

namespace hyprdeck {
    namespace {

        struct SLayoutMetrics {
            Vector2D viewSize;
            double   cardW       = 0.0;
            double   cardH       = 0.0;
            double   gap         = 0.0;
            double   y           = 0.0;
            double   specialW    = 0.0;
            double   specialH    = 0.0;
            double   specialY    = 0.0;
            double   specialGap  = 0.0;
            bool     hasSpecials = false;
        };

        struct SLayoutInputs {
            SLayoutSignature          signature;
            std::vector<PHLWORKSPACE> specialWorkspaces;
            WORKSPACEID               lastWorkspace   = 0;
            WORKSPACEID               activeNormalID  = WORKSPACE_INVALID;
            WORKSPACEID               activeSpecialID = WORKSPACE_INVALID;
        };

        bool sameLayoutSignature(const SLayoutSignature& lhs, const SLayoutSignature& rhs) {
            return lhs.monitorID == rhs.monitorID && lhs.transformedW == rhs.transformedW && lhs.transformedH == rhs.transformedH && lhs.pixelW == rhs.pixelW &&
                lhs.pixelH == rhs.pixelH && lhs.zoom == rhs.zoom && lhs.cameraX == rhs.cameraX && lhs.specialCameraX == rhs.specialCameraX &&
                lhs.resetCamera == rhs.resetCamera && lhs.suppressNextActiveCenter == rhs.suppressNextActiveCenter && lhs.selectedRow == rhs.selectedRow &&
                lhs.selectedNormalID == rhs.selectedNormalID && lhs.selectedSpecialID == rhs.selectedSpecialID && lhs.pendingSpecialID == rhs.pendingSpecialID &&
                lhs.activeNormalID == rhs.activeNormalID && lhs.activeSpecialID == rhs.activeSpecialID && lhs.lastWorkspace == rhs.lastWorkspace &&
                lhs.normalWorkspaceIDs == rhs.normalWorkspaceIDs && lhs.specialWorkspaceKeys == rhs.specialWorkspaceKeys;
        }

        std::vector<WORKSPACEID> normalWorkspaceIDs() {
            std::vector<WORKSPACEID> ids;

            for (const auto& workspace : g_pCompositor->getWorkspacesCopy()) {
                if (isNormalNumericWorkspace(workspace))
                    ids.push_back(workspace->m_id);
            }

            std::ranges::sort(ids);
            const auto duplicates = std::ranges::unique(ids);
            ids.erase(duplicates.begin(), duplicates.end());
            return ids;
        }

        std::vector<std::string> specialWorkspaceKeys(const std::vector<PHLWORKSPACE>& workspaces) {
            std::vector<std::string> keys;
            keys.reserve(workspaces.size());

            for (const auto& workspace : workspaces)
                keys.push_back(std::to_string(workspace->m_id) + ":" + workspace->m_name);

            return keys;
        }

        SLayoutInputs layoutInputs(const PHLMONITOR& monitor) {
            const auto& current           = state();
            const auto& session           = current.session;
            const auto& layout            = current.layout;
            const auto& interaction       = current.interaction;
            const auto& selection         = current.selection;
            auto        specialWorkspaces = specialWorkspacesToShow(monitor);
            const auto  lastWorkspace     = lastWorkspaceToShow(monitor);
            const auto  activeNormalID    = activeNormalWorkspaceID(monitor);
            const auto  activeSpecialID   = activeSpecialWorkspaceID(monitor);

            SLayoutInputs inputs{
                .signature = SLayoutSignature{
                    .monitorID                = monitor->m_id,
                    .transformedW             = monitor->m_transformedSize.x,
                    .transformedH             = monitor->m_transformedSize.y,
                    .pixelW                   = monitor->m_pixelSize.x,
                    .pixelH                   = monitor->m_pixelSize.y,
                    .zoom                     = session.zoom,
                    .cameraX                  = layout.cameraX,
                    .specialCameraX           = layout.specialCameraX,
                    .resetCamera              = layout.resetCamera,
                    .suppressNextActiveCenter = interaction.suppressNextActiveCenter,
                    .selectedRow              = selection.selectedRow,
                    .selectedNormalID         = selection.selectedNormalID,
                    .selectedSpecialID        = selection.selectedSpecialID,
                    .pendingSpecialID         = selection.pendingSpecialID,
                    .activeNormalID           = activeNormalID,
                    .activeSpecialID          = activeSpecialID,
                    .lastWorkspace            = lastWorkspace,
                    .normalWorkspaceIDs       = normalWorkspaceIDs(),
                    .specialWorkspaceKeys     = specialWorkspaceKeys(specialWorkspaces),
                },
                .specialWorkspaces = std::move(specialWorkspaces),
                .lastWorkspace     = lastWorkspace,
                .activeNormalID    = activeNormalID,
                .activeSpecialID   = activeSpecialID,
            };

            return inputs;
        }

        void storeCleanLayoutSignature(const SLayoutInputs& inputs) {
            auto& current   = state();
            auto& session   = current.session;
            auto& layout    = current.layout;
            auto& interaction = current.interaction;
            auto& selection = current.selection;
            auto  signature = inputs.signature;

            signature.zoom                     = session.zoom;
            signature.cameraX                  = layout.cameraX;
            signature.specialCameraX           = layout.specialCameraX;
            signature.resetCamera              = layout.resetCamera;
            signature.suppressNextActiveCenter = interaction.suppressNextActiveCenter;
            signature.selectedRow              = selection.selectedRow;
            signature.selectedNormalID         = selection.selectedNormalID;
            signature.selectedSpecialID        = selection.selectedSpecialID;
            signature.pendingSpecialID         = selection.pendingSpecialID;

            layout.signature      = std::move(signature);
            layout.signatureValid = true;
            layout.dirty          = false;
        }

        double monitorAspect(const PHLMONITOR& monitor) {
            return std::max(0.1, monitor->m_pixelSize.x / std::max(1.0, monitor->m_pixelSize.y));
        }

        SLayoutMetrics layoutMetrics(const PHLMONITOR& monitor, const bool hasSpecials) {
            const auto&  session = state().session;

            const auto   viewSize  = monitor->m_transformedSize;
            const double aspect    = monitorAspect(monitor);
            const double maxCardH  = viewSize.y * NORMAL_CARD_MAX_HEIGHT_RATIO;
            double       baseCardW = viewSize.x * NORMAL_CARD_WIDTH_RATIO;
            double       baseCardH = baseCardW / aspect;

            if (baseCardH > maxCardH) {
                baseCardH = maxCardH;
                baseCardW = baseCardH * aspect;
            }

            double cardW = baseCardW * session.zoom;
            double cardH = cardW / aspect;

            if (cardH > maxCardH) {
                cardH = maxCardH;
                cardW = cardH * aspect;
            }

            const double gap          = std::max(MIN_CARD_GAP, viewSize.x * CARD_GAP_RATIO * session.zoom);
            const double baseGap      = std::max(MIN_CARD_GAP, viewSize.x * CARD_GAP_RATIO);
            const double specialScale = session.zoom < SPECIAL_CARD_SCALE_THRESHOLD ? session.zoom / SPECIAL_CARD_SCALE_THRESHOLD : 1.0;
            const double specialH     = hasSpecials ? baseCardH * SPECIAL_CARD_HEIGHT_RATIO * specialScale : 0.0;
            const double specialW     = specialH * aspect;
            const double rowGap       = hasSpecials ? std::max(MIN_ROW_GAP, viewSize.y * ROW_GAP_RATIO) : 0.0;
            const double totalH       = hasSpecials ? cardH + rowGap + specialH : cardH;
            const double y            = hasSpecials ? (viewSize.y - totalH) / 2.0 : (viewSize.y - cardH) / 2.0;

            return SLayoutMetrics{
                .viewSize    = viewSize,
                .cardW       = cardW,
                .cardH       = cardH,
                .gap         = gap,
                .y           = y,
                .specialW    = specialW,
                .specialH    = specialH,
                .specialY    = y + cardH + rowGap,
                .specialGap  = std::max(MIN_SPECIAL_CARD_GAP, baseGap * SPECIAL_CARD_GAP_SCALE),
                .hasSpecials = hasSpecials,
            };
        }

        std::optional<size_t> workspaceIndex(const std::vector<PHLWORKSPACE>& workspaces, const WORKSPACEID id) {
            for (size_t i = 0; i < workspaces.size(); ++i) {
                if (workspaces[i] && workspaces[i]->m_id == id)
                    return i;
            }

            return std::nullopt;
        }

        void updateNormalCamera(const WORKSPACEID activeID, const WORKSPACEID lastWorkspace, const bool activeChanged, const bool suppressActiveCenter) {
            auto& current = state();
            auto& layout = current.layout;
            auto& selection = current.selection;

            if (layout.resetCamera || (activeChanged && !suppressActiveCenter)) {
                layout.cameraX     = static_cast<double>(std::max<WORKSPACEID>(1, activeID) - 1) * layout.stepX;
                layout.resetCamera = false;

                if (activeChanged)
                    selection.selectedNormalID = activeID;
            } else if (activeChanged) {
                selection.selectedNormalID = activeID;
            }

            selection.lastActiveNormalID = activeID;
            layout.cameraX              = clampCameraForCount(layout.cameraX, static_cast<size_t>(lastWorkspace));
        }

        void appendNormalCards(const PHLMONITOR& monitor, const SLayoutMetrics& metrics, const WORKSPACEID lastWorkspace) {
            auto& layout = state().layout;

            for (WORKSPACEID id = 1; id <= lastWorkspace; ++id) {
                const double worldX    = static_cast<double>(id - 1) * layout.stepX;
                const double x         = (metrics.viewSize.x / 2.0) - (metrics.cardW / 2.0) + worldX - layout.cameraX;
                auto         workspace = g_pCompositor->getWorkspaceByID(id);

                layout.cards.push_back(SWorkspaceCard{
                    .id        = id,
                    .box       = CBox{x, metrics.y, metrics.cardW, metrics.cardH},
                    .workspace = isNormalNumericWorkspace(workspace) ? workspace : nullptr,
                    .label     = std::to_string(id),
                    .special   = false,
                });
            }
        }

        void updateSpecialCards(const std::vector<PHLWORKSPACE>& workspaces, const SLayoutMetrics& metrics, const WORKSPACEID activeID, const bool resetCamera,
                                const bool activeChanged, const bool suppressActiveCenter) {
            auto& current = state();
            auto& layout = current.layout;
            auto& selection = current.selection;

            layout.specialStepX = metrics.specialW + metrics.specialGap;

            const size_t totalCards = workspaces.size();

            if (resetCamera || (activeChanged && !suppressActiveCenter)) {
                size_t activeIndex = 0;
                if (const auto index = workspaceIndex(workspaces, activeID); index)
                    activeIndex = *index;

                if (activeID != WORKSPACE_INVALID || resetCamera) {
                    layout.specialCameraX = static_cast<double>(activeIndex) * layout.specialStepX;
                    if (activeChanged)
                        selection.selectedSpecialID = activeID;
                }
            } else if (activeChanged) {
                selection.selectedSpecialID = activeID;
            }

            layout.specialCameraX = clampSpecialCamera(layout.specialCameraX, totalCards);

            for (size_t i = 0; i < workspaces.size(); ++i) {
                const auto&  workspace = workspaces[i];
                const double x         = (metrics.viewSize.x / 2.0) - (metrics.specialW / 2.0) + (static_cast<double>(i) * layout.specialStepX) - layout.specialCameraX;

                layout.specialCards.push_back(SWorkspaceCard{
                    .id        = workspace->m_id,
                    .box       = CBox{x, metrics.specialY, metrics.specialW, metrics.specialH},
                    .workspace = workspace,
                    .label     = specialWorkspaceLabel(workspace),
                    .special   = true,
                });
            }
        }

    } // namespace

    CBox expanded(CBox box, const double amount) {
        box.x -= amount;
        box.y -= amount;
        box.w += amount * 2.0;
        box.h += amount * 2.0;
        return box;
    }

    double clampCameraForCount(const double value, const size_t count) {
        if (count == 0)
            return 0.0;

        const double maxCamera = static_cast<double>(count - 1) * state().layout.stepX;
        return std::clamp(value, 0.0, maxCamera);
    }

    double clampCamera(const double value) {
        return clampCameraForCount(value, state().layout.cards.size());
    }

    double clampSpecialCamera(const double value, const size_t count) {
        if (count == 0)
            return 0.0;

        const double maxCamera = static_cast<double>(count - 1) * state().layout.specialStepX;
        return std::clamp(value, 0.0, maxCamera);
    }

    void invalidateLayout() {
        state().layout.dirty = true;
    }

    void adjustZoom(const double factor, const PHLMONITOR& monitor) {
        auto& current = state();
        auto& session = current.session;
        auto& layout  = current.layout;
        recalculateCards(monitor);

        const double normalRatio  = layout.stepX > 0.0 ? layout.cameraX / layout.stepX : 0.0;
        const double specialRatio = layout.specialStepX > 0.0 ? layout.specialCameraX / layout.specialStepX : 0.0;

        session.zoom = std::clamp(session.zoom * factor, MIN_ZOOM, MAX_ZOOM);
        invalidateLayout();

        recalculateCards(monitor);
        layout.cameraX        = normalRatio * layout.stepX;
        layout.specialCameraX = specialRatio * layout.specialStepX;
        invalidateLayout();
        recalculateCards(monitor);

        g_pHyprRenderer->damageMonitor(monitor);
    }

    void recalculateCards(const PHLMONITOR& monitor) {
        auto& current = state();
        auto& session = current.session;
        auto& interaction = current.interaction;
        auto& layout = current.layout;
        auto& selection = current.selection;

        if (!monitor) {
            layout.cards.clear();
            layout.specialCards.clear();
            layout.signatureValid = false;
            layout.dirty          = false;
            return;
        }

        const auto viewSize = monitor->m_transformedSize;
        if (viewSize.x <= 1 || viewSize.y <= 1) {
            layout.cards.clear();
            layout.specialCards.clear();
            layout.signatureValid = false;
            layout.dirty          = false;
            return;
        }

        session.zoom = std::clamp(session.zoom, MIN_ZOOM, MAX_ZOOM);

        const auto inputs = layoutInputs(monitor);
        if (!layout.dirty && layout.signatureValid && sameLayoutSignature(inputs.signature, layout.signature))
            return;

        layout.cards.clear();
        layout.specialCards.clear();

        const bool resetCamera          = layout.resetCamera;
        const auto& specialWorkspaces   = inputs.specialWorkspaces;
        const bool showSpecialRow       = !specialWorkspaces.empty();
        const auto metrics              = layoutMetrics(monitor, showSpecialRow);
        const auto lastWorkspace        = inputs.lastWorkspace;
        const auto activeNormalID       = inputs.activeNormalID;
        const auto activeSpecialID      = inputs.activeSpecialID;
        const bool normalChanged        = selection.lastActiveNormalID != WORKSPACE_INVALID && activeNormalID != selection.lastActiveNormalID;
        const bool specialChanged       = activeSpecialID != WORKSPACE_INVALID && activeSpecialID != selection.lastActiveSpecialID;
        const bool anySpecialChange     = activeSpecialID != selection.lastActiveSpecialID;
        const bool suppressActiveCenter = interaction.suppressNextActiveCenter;

        layout.stepX = metrics.cardW + metrics.gap;

        updateNormalCamera(activeNormalID, lastWorkspace, normalChanged, suppressActiveCenter);
        appendNormalCards(monitor, metrics, lastWorkspace);

        if (metrics.hasSpecials)
            updateSpecialCards(specialWorkspaces, metrics, activeSpecialID, resetCamera, specialChanged, suppressActiveCenter);
        else
            layout.specialCameraX = 0.0;

        selection.lastActiveSpecialID = activeSpecialID;

        if (normalChanged || anySpecialChange)
            interaction.suppressNextActiveCenter = false;

        ensureSelection(monitor);
        storeCleanLayoutSignature(inputs);
    }

    Vector2D cursorRenderPos(const PHLMONITOR& monitor) {
        if (!monitor)
            return {};

        return (g_pInputManager->getMouseCoordsInternal() - monitor->m_position) * monitor->m_scale;
    }

    const SWorkspaceCard* cardAt(const Vector2D& position) {
        const auto& layout = state().layout;

        for (const auto& card : layout.cards) {
            if (card.box.containsPoint(position))
                return &card;
        }

        for (const auto& card : layout.specialCards) {
            if (card.box.containsPoint(position))
                return &card;
        }

        return nullptr;
    }

    EDragRow dragRowAt(const Vector2D& position) {
        const auto& layout = state().layout;

        for (const auto& card : layout.cards) {
            if (card.box.containsPoint(position))
                return EDragRow::NORMAL;
        }

        for (const auto& card : layout.specialCards) {
            if (card.box.containsPoint(position))
                return EDragRow::SPECIAL;
        }

        if (!layout.specialCards.empty() && !layout.cards.empty() && position.y >= layout.cards.front().box.y + layout.cards.front().box.h)
            return EDragRow::SPECIAL;

        return EDragRow::NORMAL;
    }

    void centerNormalCard(const int index) {
        if (index < 0)
            return;

        auto& layout = state().layout;
        const auto next = clampCameraForCount(static_cast<double>(index) * layout.stepX, layout.cards.size());
        if (layout.cameraX == next)
            return;

        layout.cameraX = next;
        invalidateLayout();
    }

    void centerSpecialCard(const int index) {
        if (index < 0)
            return;

        auto& layout = state().layout;
        const auto next = clampSpecialCamera(static_cast<double>(index) * layout.specialStepX, layout.specialCards.size());
        if (layout.specialCameraX == next)
            return;

        layout.specialCameraX = next;
        invalidateLayout();
    }

} // namespace hyprdeck
