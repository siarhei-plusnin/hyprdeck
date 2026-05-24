#pragma once

#include "state.hpp"

#include <helpers/math/Math.hpp>

#include <cstddef>

namespace hyprdeck {

    CBox                  expanded(CBox box, double amount);
    double                clampCameraForCount(double value, size_t count);
    double                clampCamera(double value);
    double                clampSpecialCamera(double value, size_t count);
    void                  invalidateLayout();
    void                  adjustZoom(double factor, const PHLMONITOR& monitor);
    void                  recalculateCards(const PHLMONITOR& monitor);
    Vector2D              cursorRenderPos(const PHLMONITOR& monitor);

    const SWorkspaceCard* cardAt(const Vector2D& position);
    EDragRow              dragRowAt(const Vector2D& position);
    void                  centerNormalCard(int index);
    void                  centerSpecialCard(int index);

} // namespace hyprdeck
