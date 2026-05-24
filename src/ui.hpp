#pragma once

#include <helpers/Color.hpp>
#include <helpers/math/Math.hpp>
#include <render/Texture.hpp>

#include <string>
#include <string_view>

namespace hyprdeck {

    enum class ETextCacheMode {
        PERSISTENT,
        NONE,
    };

    void                 addRect(const CBox& box, const CHyprColor& color, int rounding = 0);
    void                 addTexture(const SP<Render::ITexture>& texture, const CBox& box, float alpha = 1.0F, int rounding = 0, const CBox& clipBox = {});
    SP<Render::ITexture> textTexture(std::string_view scope, const std::string& text, const CHyprColor& color, int fontSize, int weight,
                                     ETextCacheMode cacheMode = ETextCacheMode::PERSISTENT);
    void                 clearTextTextureCache();
    void                 clearTextTextureCache(std::string_view scope);

} // namespace hyprdeck
