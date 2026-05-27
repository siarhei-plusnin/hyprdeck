#include "shortcuts.hpp"

#include "colors.hpp"
#include "layout.hpp"
#include "shortcuts_menu.hpp"
#include "state.hpp"
#include "textinput_render.hpp"
#include "ui.hpp"

#include <helpers/Monitor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace hyprdeck {
    namespace {

        SP<Render::ITexture> labelTexture(const std::string& label, const int fontSize = 20, const int weight = 600, const ETextCacheMode cacheMode = ETextCacheMode::PERSISTENT) {
            return textTexture("shortcuts", label, colors::textSecondary(), fontSize, weight, cacheMode);
        }

    } // namespace

    void renderShortcutFooter(const PHLMONITOR& monitor) {
        const auto text = shortcutFooterText();
        if (text.empty())
            return;

        const auto texture = labelTexture(text, 18, 650, ETextCacheMode::NONE);
        if (!texture || !texture->ok())
            return;

        const auto viewSize = monitor->m_transformedSize;
        const CBox textBox{24.0, std::max(18.0, viewSize.y - texture->m_size.y - 22.0), texture->m_size.x, texture->m_size.y};
        addTexture(texture, textBox);
    }

    void renderShortcutMenu(const PHLMONITOR& monitor) {
        if (!state().shortcuts.open)
            return;

        auto&        shortcuts = state().shortcuts;
        const auto   actions  = filteredShortcutMenuActions();
        const auto   viewSize = monitor->m_transformedSize;
        const double rowH     = 52.0;
        const auto   title    = labelTexture("Keybindings", 30, 750);

        std::vector<SP<Render::ITexture>> keyTextures;
        std::vector<SP<Render::ITexture>> labelTextures;
        std::vector<SP<Render::ITexture>> descriptionTextures;
        keyTextures.reserve(actions.size());
        labelTextures.reserve(actions.size());
        descriptionTextures.reserve(actions.size());

        const double keyW   = shortcuts.keyWidth;
        const double labelW = shortcuts.labelWidth;
        for (const auto& action : actions) {
            auto key         = labelTexture(action.key, 21, 750);
            auto label       = labelTexture(action.label, 21, 700);
            auto description = labelTexture(action.description, 20, 500);

            keyTextures.push_back(std::move(key));
            labelTextures.push_back(std::move(label));
            descriptionTextures.push_back(std::move(description));
        }

        const double boxW = std::min(viewSize.x - 64.0, shortcuts.width);
        const double boxH = std::min(viewSize.y - 64.0, shortcuts.height);
        const CBox   box{(viewSize.x - boxW) / 2.0, (viewSize.y - boxH) / 2.0, boxW, boxH};

        addRect(expanded(box, 2.0), colors::componentBorder());
        addRect(box, colors::componentBackground());

        if (title && title->ok())
            addTexture(title, CBox{box.x + 24.0, box.y + 22.0, title->m_size.x, title->m_size.y});

        const CBox searchBox{box.x + 18.0, box.y + 68.0, box.w - 36.0, 44.0};
        addRect(searchBox, colors::componentSurface());
        renderTextInputLine("shortcuts", shortcuts.searchInput, searchBox, "Search: ", colors::textSecondary(), 23, 500);

        if (actions.empty()) {
            const auto empty = labelTexture("No matching keybindings", 22, 600);
            if (empty && empty->ok())
                addTexture(empty, CBox{box.x + 24.0, box.y + 130.0, empty->m_size.x, empty->m_size.y});
            return;
        }

        const size_t maxRows = static_cast<size_t>(std::max(1.0, std::floor((box.h - 134.0) / rowH)));
        const size_t rows    = std::min(maxRows, actions.size());
        const double labelX  = 32.0 + keyW + 44.0;
        const double descX   = labelX + labelW + 44.0;
        for (size_t i = 0; i < rows; ++i) {
            const CBox row{box.x + 18.0, box.y + 126.0 + (static_cast<double>(i) * rowH), box.w - 36.0, rowH - 8.0};
            addRect(row, i % 2 == 0 ? colors::shortcutRowEven() : colors::shortcutRowOdd());

            const auto& key = keyTextures[i];
            if (key && key->ok())
                addTexture(key, CBox{row.x + 14.0, row.y + ((row.h - key->m_size.y) / 2.0), key->m_size.x, key->m_size.y});

            const auto& label = labelTextures[i];
            if (label && label->ok())
                addTexture(label, CBox{row.x + labelX, row.y + ((row.h - label->m_size.y) / 2.0), label->m_size.x, label->m_size.y});

            const auto& description = descriptionTextures[i];
            if (description && description->ok())
                addTexture(description, CBox{row.x + descX, row.y + ((row.h - description->m_size.y) / 2.0), description->m_size.x, description->m_size.y});
        }
    }

} // namespace hyprdeck
