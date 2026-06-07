#pragma once

#include <helpers/Color.hpp>
#include <helpers/math/Math.hpp>
#include <render/Texture.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hyprdeck {

    enum class ETextCacheMode {
        PERSISTENT,
        NONE,
    };

    struct SRenderCache {
        std::unordered_map<std::string, SP<Render::ITexture>> labelTextures;
        SP<Render::ITexture>                                  cursorTexture;
        Vector2D                                              cursorHotspot = {};
        Vector2D                                              cursorSize    = {18, 24};
    };

    class CRenderServices {
      public:
        CBox                 expandedBox(CBox box, double amount) const;
        void                 addRect(const CBox& box, const CHyprColor& color, int rounding = 0, const CBox& clipBox = {});
        void                 addTexture(const SP<Render::ITexture>& texture, const CBox& box, float alpha = 1.0F, int rounding = 0, const CBox& clipBox = {});
        void                 pushOpacity(float opacity);
        void                 popOpacity();
        SP<Render::ITexture> textTexture(std::string_view scope, const std::string& text, const CHyprColor& color, int fontSize, int weight,
                                          ETextCacheMode cacheMode = ETextCacheMode::PERSISTENT);
        void                 clearTextTextureCache();
        void                 clearTextTextureCache(std::string_view scope);
        void                 updateCursorCache();
        void                 clearCursorCache();

        const SRenderCache& cache() const;

      private:
        float effectiveOpacity() const;

        SRenderCache m_cache;
        std::vector<float> m_opacityStack;
    };

} // namespace hyprdeck
