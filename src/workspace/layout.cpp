#include "layout.hpp"

#include "constants.hpp"
#include "plugin.hpp"
#include "workspace_filter.hpp"
#include "workspace_filter_match.hpp"
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
            std::vector<WORKSPACEID>  normalWorkspaceIDs;
            std::vector<PHLWORKSPACE> specialWorkspaces;
            WORKSPACEID               lastWorkspace   = 0;
            WORKSPACEID               activeNormalID  = WORKSPACE_INVALID;
            WORKSPACEID               activeSpecialID = WORKSPACE_INVALID;
        };

        bool sameLayoutSignature(const SLayoutSignature& lhs, const SLayoutSignature& rhs) {
            return lhs.monitorID == rhs.monitorID && lhs.transformedW == rhs.transformedW && lhs.transformedH == rhs.transformedH && lhs.pixelW == rhs.pixelW &&
                lhs.pixelH == rhs.pixelH && lhs.zoom == rhs.zoom && lhs.cameraX == rhs.cameraX && lhs.specialCameraX == rhs.specialCameraX && lhs.resetCamera == rhs.resetCamera &&
                lhs.selectedRow == rhs.selectedRow && lhs.selectedNormalID == rhs.selectedNormalID && lhs.selectedSpecialID == rhs.selectedSpecialID &&
                lhs.lastWorkspace == rhs.lastWorkspace && lhs.normalWorkspaceIDs == rhs.normalWorkspaceIDs && lhs.specialWorkspaceKeys == rhs.specialWorkspaceKeys &&
                lhs.workspaceFilter == rhs.workspaceFilter;
        }

        std::vector<WORKSPACEID> defaultNormalWorkspaceIDs(const WORKSPACEID lastWorkspace) {
            std::vector<WORKSPACEID> ids;
            ids.reserve(static_cast<size_t>(std::max<WORKSPACEID>(0, lastWorkspace)));

            for (WORKSPACEID id = 1; id <= lastWorkspace; ++id)
                ids.push_back(id);

            return ids;
        }

        std::vector<std::string> specialWorkspaceKeys(const std::vector<PHLWORKSPACE>& workspaces) {
            std::vector<std::string> keys;
            keys.reserve(workspaces.size());

            for (const auto& workspace : workspaces)
                keys.push_back(std::to_string(workspace->m_id) + ":" + workspace->m_name);

            return keys;
        }

        SLayoutInputs layoutInputs(const PHLMONITOR& monitor, const SLayoutState& layout) {
            const auto  selection         = activePlugin()->selection().snapshot();
            const auto& overview          = activePlugin()->overview();
            const auto& workspaces        = activePlugin()->workspaces();
            const auto  lastWorkspace     = workspaces.lastWorkspaceToShow(monitor);
            auto        rows              = activePlugin()->workspaceFilterMatcher().apply(monitor, defaultNormalWorkspaceIDs(lastWorkspace), workspaces.specialWorkspacesToShow(monitor));
            auto       normalIDs         = std::move(rows.normalWorkspaceIDs);
            auto       specialWorkspaces = std::move(rows.specialWorkspaces);
            const auto activeNormalID    = workspaces.activeNormalWorkspaceID(monitor);
            const auto activeSpecialID   = workspaces.activeSpecialWorkspaceID(monitor);

            SLayoutInputs inputs{
                .signature =
                    SLayoutSignature{
                        .monitorID                = monitor->m_id,
                        .transformedW             = monitor->m_transformedSize.x,
                        .transformedH             = monitor->m_transformedSize.y,
                        .pixelW                   = monitor->m_pixelSize.x,
                        .pixelH                   = monitor->m_pixelSize.y,
                        .zoom                     = overview.zoom(),
                        .cameraX                  = layout.cameraX,
                        .specialCameraX           = layout.specialCameraX,
                        .resetCamera              = layout.resetCamera,
                        .selectedRow              = selection.selectedRow,
                        .selectedNormalID         = selection.selectedNormalID,
                        .selectedSpecialID        = selection.selectedSpecialID,
                        .activeNormalID           = activeNormalID,
                        .activeSpecialID          = activeSpecialID,
                        .lastWorkspace            = lastWorkspace,
                        .normalWorkspaceIDs       = normalIDs,
                        .specialWorkspaceKeys     = specialWorkspaceKeys(specialWorkspaces),
                        .workspaceFilter          = std::string{activePlugin()->workspaceFilter().text()},
                    },
                .normalWorkspaceIDs = std::move(normalIDs),
                .specialWorkspaces  = std::move(specialWorkspaces),
                .lastWorkspace      = lastWorkspace,
                .activeNormalID     = activeNormalID,
                .activeSpecialID    = activeSpecialID,
            };

            return inputs;
        }

        void storeCleanLayoutSignature(const SLayoutInputs& inputs, SLayoutState& layout) {
            const auto selection   = activePlugin()->selection().snapshot();
            const auto& overview = activePlugin()->overview();
            auto  signature   = inputs.signature;

            signature.zoom                     = overview.zoom();
            signature.cameraX                  = layout.cameraX;
            signature.specialCameraX           = layout.specialCameraX;
            signature.resetCamera              = layout.resetCamera;
            signature.selectedRow              = selection.selectedRow;
            signature.selectedNormalID         = selection.selectedNormalID;
            signature.selectedSpecialID        = selection.selectedSpecialID;

            layout.signature      = std::move(signature);
            layout.signatureValid = true;
            layout.dirty          = false;
        }

        double monitorAspect(const PHLMONITOR& monitor) {
            return std::max(0.1, monitor->m_pixelSize.x / std::max(1.0, monitor->m_pixelSize.y));
        }

        SLayoutMetrics layoutMetrics(const PHLMONITOR& monitor, const bool hasNormals, const bool hasSpecials) {
            const double zoom = activePlugin()->overview().zoom();

            const auto   viewSize  = monitor->m_transformedSize;
            const double aspect    = monitorAspect(monitor);
            const double maxCardH  = viewSize.y * NORMAL_CARD_MAX_HEIGHT_RATIO;
            double       baseCardW = viewSize.x * NORMAL_CARD_WIDTH_RATIO;
            double       baseCardH = baseCardW / aspect;

            if (baseCardH > maxCardH) {
                baseCardH = maxCardH;
                baseCardW = baseCardH * aspect;
            }

            double cardW = baseCardW * zoom;
            double cardH = cardW / aspect;

            if (cardH > maxCardH) {
                cardH = maxCardH;
                cardW = cardH * aspect;
            }

            const double gap          = std::max(MIN_CARD_GAP, viewSize.x * CARD_GAP_RATIO * zoom);
            const double baseGap      = std::max(MIN_CARD_GAP, viewSize.x * CARD_GAP_RATIO);
            const double specialScale = zoom < SPECIAL_CARD_SCALE_THRESHOLD ? zoom / SPECIAL_CARD_SCALE_THRESHOLD : 1.0;
            const double specialH     = hasSpecials ? baseCardH * SPECIAL_CARD_HEIGHT_RATIO * specialScale : 0.0;
            const double specialW     = specialH * aspect;
            const double rowGap       = hasNormals && hasSpecials ? std::max(MIN_ROW_GAP, viewSize.y * ROW_GAP_RATIO) : 0.0;
            const double totalH       = (hasNormals ? cardH : 0.0) + rowGap + specialH;
            const double y            = (viewSize.y - totalH) / 2.0;

            return SLayoutMetrics{
                .viewSize    = viewSize,
                .cardW       = cardW,
                .cardH       = cardH,
                .gap         = gap,
                .y           = y,
                .specialW    = specialW,
                .specialH    = specialH,
                .specialY    = hasNormals ? y + cardH + rowGap : y,
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

        std::optional<size_t> workspaceIDIndex(const std::vector<WORKSPACEID>& ids, const WORKSPACEID id) {
            for (size_t i = 0; i < ids.size(); ++i) {
                if (ids[i] == id)
                    return i;
            }

            return std::nullopt;
        }

        void updateNormalCamera(SLayoutState& layout, const std::vector<WORKSPACEID>& ids, const WORKSPACEID activeID, const bool activeChanged, const bool centerActiveChange) {
            if (ids.empty()) {
                layout.cameraX     = 0.0;
                layout.resetCamera = false;
                activePlugin()->selection().setSelectedNormalID(WORKSPACE_INVALID);
                activePlugin()->selection().setLastActiveNormalID(activeID);
                return;
            }

            const auto activeIndex = workspaceIDIndex(ids, activeID);
            const auto targetIndex = activeIndex.value_or(0);
            const auto targetID    = activeIndex ? activeID : ids.front();

            if (layout.resetCamera || (activeChanged && centerActiveChange)) {
                layout.cameraX     = static_cast<double>(targetIndex) * layout.stepX;
                layout.resetCamera = false;

                if (activeChanged)
                    activePlugin()->selection().setSelectedNormalID(targetID);
            } else if (activeChanged) {
                activePlugin()->selection().setSelectedNormalID(targetID);
            }

            activePlugin()->selection().setLastActiveNormalID(activeID);
            layout.cameraX               = activePlugin()->layout().clampCameraForCount(layout.cameraX, ids.size());
        }

        void appendNormalCards(SLayoutState& layout, const PHLMONITOR& monitor, const SLayoutMetrics& metrics, const std::vector<WORKSPACEID>& ids) {
            for (size_t i = 0; i < ids.size(); ++i) {
                const auto   id        = ids[i];
                const double worldX    = static_cast<double>(i) * layout.stepX;
                const double x         = (metrics.viewSize.x / 2.0) - (metrics.cardW / 2.0) + worldX - layout.cameraX;
                auto         workspace = activePlugin()->hyprland().workspaceByID(id);

                layout.cards.push_back(SWorkspaceCard{
                    .id        = id,
                    .box       = CBox{x, metrics.y, metrics.cardW, metrics.cardH},
                    .workspace = activePlugin()->workspaces().isNormalWorkspace(workspace) ? workspace : nullptr,
                    .label     = std::to_string(id),
                    .special   = false,
                });
            }
        }

        void updateSpecialCards(SLayoutState& layout, const std::vector<PHLWORKSPACE>& workspaces, const SLayoutMetrics& metrics, const WORKSPACEID activeID, const bool resetCamera,
                                const bool activeChanged, const bool centerActiveChange) {
            layout.specialStepX = metrics.specialW + metrics.specialGap;

            const size_t totalCards = workspaces.size();

            if (resetCamera || (activeChanged && centerActiveChange)) {
                size_t activeIndex = 0;
                if (const auto index = workspaceIndex(workspaces, activeID); index)
                    activeIndex = *index;

                if (activeID != WORKSPACE_INVALID || resetCamera) {
                    layout.specialCameraX = static_cast<double>(activeIndex) * layout.specialStepX;
                    if (activeChanged)
                        activePlugin()->selection().setSelectedSpecialID(activeID);
                }
            } else if (activeChanged) {
                activePlugin()->selection().setSelectedSpecialID(activeID);
            }

            layout.specialCameraX = activePlugin()->layout().clampSpecialCamera(layout.specialCameraX, totalCards);

            for (size_t i = 0; i < workspaces.size(); ++i) {
                const auto&  workspace = workspaces[i];
                const double x         = (metrics.viewSize.x / 2.0) - (metrics.specialW / 2.0) + (static_cast<double>(i) * layout.specialStepX) - layout.specialCameraX;

                layout.specialCards.push_back(SWorkspaceCard{
                    .id        = workspace->m_id,
                    .box       = CBox{x, metrics.specialY, metrics.specialW, metrics.specialH},
                    .workspace = workspace,
                    .label     = activePlugin()->workspaces().specialWorkspaceLabel(workspace),
                    .special   = true,
                });
            }
        }

    } // namespace

    const std::vector<SWorkspaceCard>& CWorkspaceLayoutController::cards() const {
        return m_state.cards;
    }

    const std::vector<SWorkspaceCard>& CWorkspaceLayoutController::specialCards() const {
        return m_state.specialCards;
    }

    bool CWorkspaceLayoutController::cardsEmpty() const {
        return m_state.cards.empty();
    }

    bool CWorkspaceLayoutController::specialCardsEmpty() const {
        return m_state.specialCards.empty();
    }

    size_t CWorkspaceLayoutController::cardCount() const {
        return m_state.cards.size();
    }

    size_t CWorkspaceLayoutController::specialCardCount() const {
        return m_state.specialCards.size();
    }

    double CWorkspaceLayoutController::cameraX() const {
        return m_state.cameraX;
    }

    double CWorkspaceLayoutController::specialCameraX() const {
        return m_state.specialCameraX;
    }

    void CWorkspaceLayoutController::setCameraX(const double value) {
        m_state.cameraX = value;
    }

    void CWorkspaceLayoutController::setSpecialCameraX(const double value) {
        m_state.specialCameraX = value;
    }

    void CWorkspaceLayoutController::setResetCamera(const bool resetCamera) {
        m_state.resetCamera = resetCamera;
    }

    void CWorkspaceLayoutController::clearCards() {
        m_state.cards.clear();
        m_state.specialCards.clear();
    }

    double CWorkspaceLayoutController::clampCameraForCount(const double value, const size_t count) const {
        if (count == 0)
            return 0.0;

        const double maxCamera = static_cast<double>(count - 1) * m_state.stepX;
        return std::clamp(value, 0.0, maxCamera);
    }

    double CWorkspaceLayoutController::clampCamera(const double value) const {
        return clampCameraForCount(value, m_state.cards.size());
    }

    double CWorkspaceLayoutController::clampSpecialCamera(const double value, const size_t count) const {
        if (count == 0)
            return 0.0;

        const double maxCamera = static_cast<double>(count - 1) * m_state.specialStepX;
        return std::clamp(value, 0.0, maxCamera);
    }

    void CWorkspaceLayoutController::invalidate() {
        m_state.dirty = true;
    }

    void CWorkspaceLayoutController::adjustZoom(const double factor, const PHLMONITOR& monitor) {
        auto& overview = activePlugin()->overview();
        recalculateCards(monitor);

        const double normalRatio  = m_state.stepX > 0.0 ? m_state.cameraX / m_state.stepX : 0.0;
        const double specialRatio = m_state.specialStepX > 0.0 ? m_state.specialCameraX / m_state.specialStepX : 0.0;
        const double targetZoom   = std::clamp(overview.zoom() * factor, MIN_ZOOM, MAX_ZOOM);

        activePlugin()->animations().cancelZoomAnimation();
        if (activePlugin()->animations().animateZoom(overview.zoom(), targetZoom, normalRatio, specialRatio, monitor)) {
            activePlugin()->hyprland().damageMonitor(monitor);
            return;
        }

        applyZoom(targetZoom, normalRatio, specialRatio, monitor);
        activePlugin()->hyprland().damageMonitor(monitor);
    }

    void CWorkspaceLayoutController::applyZoom(const double zoom, const double normalCameraRatio, const double specialCameraRatio, const PHLMONITOR& monitor) {
        auto& overview = activePlugin()->overview();

        overview.setZoom(zoom);
        invalidate();

        recalculateCards(monitor);
        m_state.cameraX        = normalCameraRatio * m_state.stepX;
        m_state.specialCameraX = specialCameraRatio * m_state.specialStepX;
        invalidate();
        recalculateCards(monitor);
    }

    void CWorkspaceLayoutController::recalculateCards(const PHLMONITOR& monitor) {
        const auto selection = activePlugin()->selection().snapshot();
        auto& overview    = activePlugin()->overview();

        const auto viewSize = monitor->m_transformedSize;
        if (viewSize.x <= 1 || viewSize.y <= 1) {
            m_state.cards.clear();
            m_state.specialCards.clear();
            m_state.signatureValid = false;
            m_state.dirty          = false;
            return;
        }

        overview.setZoom(overview.zoom());

        const auto inputs = layoutInputs(monitor, m_state);
        if (!m_state.dirty && m_state.signatureValid && sameLayoutSignature(inputs.signature, m_state.signature))
            return;

        m_state.cards.clear();
        m_state.specialCards.clear();

        const bool  resetCamera          = m_state.resetCamera;
        const auto& normalWorkspaceIDs   = inputs.normalWorkspaceIDs;
        const auto& specialWorkspaces    = inputs.specialWorkspaces;
        const bool  showNormalRow        = !normalWorkspaceIDs.empty();
        const bool  showSpecialRow       = !specialWorkspaces.empty();
        const auto  metrics              = layoutMetrics(monitor, showNormalRow, showSpecialRow);
        const auto  activeNormalID       = inputs.activeNormalID;
        const auto  activeSpecialID      = inputs.activeSpecialID;
        const bool  normalChanged        = selection.lastActiveNormalID != WORKSPACE_INVALID && activeNormalID != selection.lastActiveNormalID;
        const bool  specialChanged       = activeSpecialID != WORKSPACE_INVALID && activeSpecialID != selection.lastActiveSpecialID;
        const bool  centerActiveNormal   = selection.selectedNormalID != activeNormalID;
        const bool  centerActiveSpecial  = selection.selectedSpecialID != activeSpecialID;

        m_state.stepX = metrics.cardW + metrics.gap;

        updateNormalCamera(m_state, normalWorkspaceIDs, activeNormalID, normalChanged, centerActiveNormal);
        appendNormalCards(m_state, monitor, metrics, normalWorkspaceIDs);

        if (metrics.hasSpecials)
            updateSpecialCards(m_state, specialWorkspaces, metrics, activeSpecialID, resetCamera, specialChanged, centerActiveSpecial);
        else
            m_state.specialCameraX = 0.0;

        activePlugin()->selection().setLastActiveSpecialID(activeSpecialID);

        activePlugin()->selection().ensureSelection(monitor);
        storeCleanLayoutSignature(inputs, m_state);
    }

    Vector2D CWorkspaceLayoutController::cursorRenderPos(const PHLMONITOR& monitor) const {
        return (activePlugin()->hyprland().mouseCoords() - monitor->m_position) * monitor->m_scale;
    }

    const SWorkspaceCard* CWorkspaceLayoutController::cardAt(const Vector2D& position) const {
        for (const auto& card : m_state.cards) {
            if (card.box.containsPoint(position))
                return &card;
        }

        for (const auto& card : m_state.specialCards) {
            if (card.box.containsPoint(position))
                return &card;
        }

        return nullptr;
    }

    EDragRow CWorkspaceLayoutController::dragRowAt(const Vector2D& position) const {
        for (const auto& card : m_state.cards) {
            if (card.box.containsPoint(position))
                return EDragRow::NORMAL;
        }

        for (const auto& card : m_state.specialCards) {
            if (card.box.containsPoint(position))
                return EDragRow::SPECIAL;
        }

        if (!m_state.specialCards.empty() && !m_state.cards.empty() && position.y >= m_state.cards.front().box.y + m_state.cards.front().box.h)
            return EDragRow::SPECIAL;

        return EDragRow::NORMAL;
    }

    void CWorkspaceLayoutController::centerNormalCard(const int index) {
        if (index < 0)
            return;

        const auto next = clampCameraForCount(static_cast<double>(index) * m_state.stepX, m_state.cards.size());
        if (m_state.cameraX == next)
            return;

        if (const auto monitor = activePlugin()->overview().monitor(); activePlugin()->animations().animateNormalCamera(m_state.cameraX, next, monitor))
            return;

        m_state.cameraX = next;
        invalidate();
    }

    void CWorkspaceLayoutController::centerSpecialCard(const int index) {
        if (index < 0)
            return;

        const auto next = clampSpecialCamera(static_cast<double>(index) * m_state.specialStepX, m_state.specialCards.size());
        if (m_state.specialCameraX == next)
            return;

        if (const auto monitor = activePlugin()->overview().monitor(); activePlugin()->animations().animateSpecialCamera(m_state.specialCameraX, next, monitor))
            return;

        m_state.specialCameraX = next;
        invalidate();
    }

} // namespace hyprdeck
