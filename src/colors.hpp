#pragma once

#include <helpers/Color.hpp>

namespace hyprdeck::colors {

    inline CHyprColor accent() {
        return CHyprColor(0.45F, 0.66F, 1.0F, 0.92F);
    }

    inline CHyprColor accentOpaque() {
        return CHyprColor(0.45F, 0.66F, 1.0F, 1.0F);
    }

    inline CHyprColor accentSubtle() {
        return CHyprColor(0.45F, 0.66F, 1.0F, 0.50F);
    }

    inline CHyprColor textPrimary() {
        return CHyprColor(1.0F, 1.0F, 1.0F, 0.92F);
    }

    inline CHyprColor textSecondary() {
        return CHyprColor(0.82F, 0.84F, 0.86F, 0.96F);
    }

    inline CHyprColor opaqueBlack() {
        return CHyprColor(0.0F, 0.0F, 0.0F, 1.0F);
    }

    inline CHyprColor overviewScrim() {
        return CHyprColor(0.0F, 0.0F, 0.0F, 0.58F);
    }

    inline CHyprColor labelBackdrop() {
        return CHyprColor(0.0F, 0.0F, 0.0F, 0.45F);
    }

    inline CHyprColor workspacePreviewBackground() {
        return CHyprColor(0.08F, 0.085F, 0.10F, 1.0F);
    }

    inline CHyprColor actionCardBackground() {
        return CHyprColor(0.10F, 0.11F, 0.14F, 0.95F);
    }

    inline CHyprColor whiteOverlay(const float alpha) {
        return CHyprColor(1.0F, 1.0F, 1.0F, alpha);
    }

    inline CHyprColor inactiveCardBorder() {
        return whiteOverlay(0.12F);
    }

    inline CHyprColor actionCardBorder() {
        return whiteOverlay(0.14F);
    }

    inline CHyprColor actionCardSelectedBorder() {
        return whiteOverlay(0.24F);
    }

    inline CHyprColor actionCardSelectionGlow() {
        return whiteOverlay(0.28F);
    }

    inline CHyprColor activeCardSelectionGlow() {
        return whiteOverlay(0.28F);
    }

    inline CHyprColor inactiveCardSelectionGlow() {
        return whiteOverlay(0.34F);
    }

    inline CHyprColor inputBackground() {
        return whiteOverlay(0.10F);
    }

    inline CHyprColor selectedPromptRow() {
        return whiteOverlay(0.14F);
    }

    inline CHyprColor shortcutRowEven() {
        return whiteOverlay(0.055F);
    }

    inline CHyprColor shortcutRowOdd() {
        return whiteOverlay(0.025F);
    }

    inline CHyprColor fallbackCursor() {
        return whiteOverlay(0.95F);
    }

} // namespace hyprdeck::colors
