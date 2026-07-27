#pragma once

#include <helpers/Color.hpp>

#include <array>
#include <cstdint>
#include <string_view>

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

    inline CHyprColor rgb(const std::uint32_t value) {
        return CHyprColor(static_cast<float>((value >> 16U) & 0xFFU) / 255.0F, static_cast<float>((value >> 8U) & 0xFFU) / 255.0F, static_cast<float>(value & 0xFFU) / 255.0F,
                          1.0F);
    }

    inline CHyprColor automaticOutputColor(const std::string_view outputName) {
        static const std::array<CHyprColor, 8> PALETTE = {
            CHyprColor(0.45F, 0.66F, 1.0F, 1.0F),  CHyprColor(0.36F, 0.83F, 0.94F, 1.0F), CHyprColor(0.38F, 0.82F, 0.59F, 1.0F), CHyprColor(0.95F, 0.78F, 0.30F, 1.0F),
            CHyprColor(0.94F, 0.55F, 0.27F, 1.0F), CHyprColor(1.0F, 0.42F, 0.42F, 1.0F),  CHyprColor(0.94F, 0.38F, 0.57F, 1.0F), CHyprColor(0.73F, 0.53F, 0.99F, 1.0F),
        };

        std::uint64_t hash = 14695981039346656037ULL;
        for (const unsigned char character : outputName) {
            hash ^= character;
            hash *= 1099511628211ULL;
        }

        return PALETTE[hash % PALETTE.size()];
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

    inline CHyprColor whiteOverlay(const float alpha) {
        return CHyprColor(1.0F, 1.0F, 1.0F, alpha);
    }

    inline CHyprColor inactiveCardBorder() {
        return whiteOverlay(0.12F);
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

    inline CHyprColor componentBackground() {
        return CHyprColor(0.0F, 0.0F, 0.0F, 1.0F);
    }

    inline CHyprColor componentSurface() {
        return CHyprColor(0.10F, 0.105F, 0.12F, 1.0F);
    }

    inline CHyprColor componentSelected() {
        return CHyprColor(0.18F, 0.185F, 0.20F, 1.0F);
    }

    inline CHyprColor textCursor() {
        return CHyprColor(0.34F, 0.35F, 0.38F, 1.0F);
    }

    inline CHyprColor componentBorder() {
        return CHyprColor(0.28F, 0.29F, 0.31F, 1.0F);
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
