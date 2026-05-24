#include "ui.hpp"

#include "config.hpp"
#include "state.hpp"

#include <hyprutils/memory/UniquePtr.hpp>
#include <render/Renderer.hpp>
#include <render/pass/RectPassElement.hpp>
#include <render/pass/TexPassElement.hpp>

#include <utility>

using Hyprutils::Memory::makeUnique;

namespace hyprdeck {

    void addRect(const CBox& box, const CHyprColor& color, const int rounding) {
        CRectPassElement::SRectData data;
        data.box   = box;
        data.color = color;
        data.round = rounding;

        g_pHyprRenderer->m_renderPass.add(makeUnique<CRectPassElement>(data));
    }

    void addTexture(const SP<Render::ITexture>& texture, const CBox& box, const float alpha, const int rounding, const CBox& clipBox) {
        if (!texture || !texture->ok())
            return;

        CTexPassElement::SRenderData data;
        data.tex     = texture;
        data.box     = box;
        data.a       = alpha;
        data.round   = rounding;
        data.clipBox = clipBox;

        g_pHyprRenderer->m_renderPass.add(makeUnique<CTexPassElement>(std::move(data)));
    }

    SP<Render::ITexture> textTexture(std::string_view scope, const std::string& text, const CHyprColor& color, const int fontSize, const int weight,
                                     const ETextCacheMode cacheMode) {
        const auto fontFamily = configuredFontFamily();
        if (cacheMode == ETextCacheMode::NONE)
            return g_pHyprRenderer->renderText(text, color, fontSize, false, fontFamily, 0, weight);

        auto&      textures = state().renderCache.labelTextures;
        const auto key      = std::string{scope} + "#" + fontFamily + "#" + std::to_string(fontSize) + "#" + std::to_string(weight) + "#" + text;
        if (const auto it = textures.find(key); it != textures.end())
            return it->second;

        auto texture = g_pHyprRenderer->renderText(text, color, fontSize, false, fontFamily, 0, weight);
        textures.emplace(key, texture);
        return texture;
    }

    void clearTextTextureCache() {
        state().renderCache.labelTextures.clear();
    }

    void clearTextTextureCache(const std::string_view scope) {
        auto&      textures = state().renderCache.labelTextures;
        const auto prefix   = std::string{scope} + "#";

        std::erase_if(textures, [&](const auto& entry) { return entry.first.starts_with(prefix); });
    }

} // namespace hyprdeck
