#pragma once

#include "runtime_types.hpp"

#include <helpers/math/Math.hpp>

#include <cstddef>
#include <vector>

namespace hyprdeck {

    class CWorkspaceLayoutController {
      public:
        const std::vector<SWorkspaceCard>& cards() const;
        const std::vector<SWorkspaceCard>& specialCards() const;
        bool                              cardsEmpty() const;
        bool                              specialCardsEmpty() const;
        size_t                            cardCount() const;
        size_t                            specialCardCount() const;
        double                            cameraX() const;
        double                            specialCameraX() const;
        void                              setCameraX(double value);
        void                              setSpecialCameraX(double value);
        void                              setResetCamera(bool resetCamera);
        void                              clearCards();

        double                clampCameraForCount(double value, size_t count) const;
        double                clampCamera(double value) const;
        double                clampSpecialCamera(double value, size_t count) const;
        void                  invalidate();
        void                  adjustZoom(double factor, const PHLMONITOR& monitor);
        void                  recalculateCards(const PHLMONITOR& monitor);
        Vector2D              cursorRenderPos(const PHLMONITOR& monitor) const;
        const SWorkspaceCard* cardAt(const Vector2D& position) const;
        EDragRow              dragRowAt(const Vector2D& position) const;
        void                  centerNormalCard(int index);
        void                  centerSpecialCard(int index);

      private:
        SLayoutState m_state;
    };

} // namespace hyprdeck
