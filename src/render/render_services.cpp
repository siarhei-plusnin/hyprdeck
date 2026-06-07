#include "render_services.hpp"

#include "plugin.hpp"

#include <algorithm>
#include <utility>

namespace hyprdeck {

    CBox CRenderServices::expandedBox(CBox box, const double amount) const {
        box.x -= amount;
        box.y -= amount;
        box.w += amount * 2.0;
        box.h += amount * 2.0;
        return box;
    }

    float CRenderServices::effectiveOpacity() const {
        float opacity = 1.0F;
        for (const auto value : m_opacityStack)
            opacity *= value;

        return std::clamp(opacity, 0.0F, 1.0F);
    }

    void CRenderServices::addRect(const CBox& box, const CHyprColor& color, const int rounding, const CBox& clipBox) {
        CRectPassElement::SRectData data;
        data.box     = box;
        data.color   = color.modifyA(color.a * effectiveOpacity());
        data.round   = rounding;
        data.clipBox = clipBox;

        activePlugin()->hyprland().addRectPass(std::move(data));
    }

    void CRenderServices::addTexture(const SP<Render::ITexture>& texture, const CBox& box, const float alpha, const int rounding, const CBox& clipBox) {
        if (!texture || !texture->ok())
            return;

        CTexPassElement::SRenderData data;
        data.tex     = texture;
        data.box     = box;
        data.a       = alpha * effectiveOpacity();
        data.round   = rounding;
        data.clipBox = clipBox;

        activePlugin()->hyprland().addTexturePass(std::move(data));
    }

    void CRenderServices::pushOpacity(const float opacity) {
        m_opacityStack.push_back(std::clamp(opacity, 0.0F, 1.0F));
    }

    void CRenderServices::popOpacity() {
        if (!m_opacityStack.empty())
            m_opacityStack.pop_back();
    }

    SP<Render::ITexture> CRenderServices::textTexture(std::string_view scope, const std::string& text, const CHyprColor& color, const int fontSize, const int weight,
                                                      const ETextCacheMode cacheMode) {
        const auto fontFamily = activePlugin()->config().fontFamily();
        if (cacheMode == ETextCacheMode::NONE)
            return activePlugin()->hyprland().renderText(text, color, fontSize, fontFamily, weight);

        const auto key = std::string{scope} + "#" + fontFamily + "#" + std::to_string(fontSize) + "#" + std::to_string(weight) + "#" + text;
        if (const auto it = m_cache.labelTextures.find(key); it != m_cache.labelTextures.end())
            return it->second;

        auto texture = activePlugin()->hyprland().renderText(text, color, fontSize, fontFamily, weight);
        m_cache.labelTextures.emplace(key, texture);
        return texture;
    }

    void CRenderServices::clearTextTextureCache() {
        m_cache.labelTextures.clear();
    }

    void CRenderServices::clearTextTextureCache(const std::string_view scope) {
        const auto prefix = std::string{scope} + "#";
        std::erase_if(m_cache.labelTextures, [&](const auto& entry) { return entry.first.starts_with(prefix); });
    }

    void CRenderServices::updateCursorCache() {
        const auto texture = activePlugin()->hyprland().currentCursorTexture();
        if (!texture || !texture->ok())
            return;

        const auto hotspot = activePlugin()->hyprland().currentCursorHotspot();
        const auto size    = activePlugin()->hyprland().cursorSizeLogical();
        if (size.x <= 0 || size.y <= 0)
            return;

        m_cache.cursorTexture = texture;
        m_cache.cursorHotspot = hotspot;
        m_cache.cursorSize    = size;
    }

    void CRenderServices::clearCursorCache() {
        m_cache.cursorTexture.reset();
    }

    const SRenderCache& CRenderServices::cache() const {
        return m_cache;
    }

} // namespace hyprdeck
