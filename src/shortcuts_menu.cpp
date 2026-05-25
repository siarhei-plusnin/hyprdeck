#include "shortcuts_menu.hpp"

#include "colors.hpp"
#include "shortcut_catalog.hpp"
#include "state.hpp"
#include "strings.hpp"
#include "textinput.hpp"
#include "ui.hpp"

#include <helpers/Monitor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <string>

namespace hyprdeck {
    namespace {

        SP<Render::ITexture> labelTexture(const std::string& label, const int fontSize = 20, const int weight = 600, const ETextCacheMode cacheMode = ETextCacheMode::PERSISTENT) {
            return textTexture("shortcuts", label, colors::textSecondary(), fontSize, weight, cacheMode);
        }

    } // namespace

    std::vector<SShortcutAction> filteredShortcutMenuActions() {
        auto        actions = currentShortcutActions();
        const auto& query   = state().shortcuts.searchInput.text;
        if (query.empty())
            return actions;

        const auto loweredQuery = strings::lower(query);
        std::erase_if(actions, [&](const auto& action) {
            return !strings::containsLowered(action.key, loweredQuery) && !strings::containsLowered(action.label, loweredQuery) &&
                !strings::containsLowered(action.description, loweredQuery);
        });
        return actions;
    }

    std::string shortcutFooterText() {
        std::string text;
        for (const auto& action : footerShortcutActions()) {
            if (!text.empty())
                text += "  |  ";

            text += action.label + ": " + action.key;
        }

        return text;
    }

    std::string shortcutSearchLabel() {
        const auto& input = state().shortcuts.searchInput;
        return input.text.empty() ? std::string{"Search shortcuts"} : "Search: " + input.withCursor();
    }

    void measureShortcutMenu(const PHLMONITOR& monitor) {
        if (!monitor)
            return;

        const auto   actions  = currentShortcutActions();
        auto&        shortcuts = state().shortcuts;
        const auto   viewSize = monitor->m_transformedSize;
        const double rowH     = 52.0;
        const auto   title    = labelTexture("Keybindings", 30, 750);
        const auto   search   = labelTexture(shortcutSearchLabel(), 23, shortcuts.searchInput.text.empty() ? 500 : 700, ETextCacheMode::NONE);

        double keyW         = 0.0;
        double labelW       = 0.0;
        double descriptionW = 0.0;
        for (const auto& action : actions) {
            const auto key         = labelTexture(action.key, 21, 750);
            const auto label       = labelTexture(action.label, 21, 700);
            const auto description = labelTexture(action.description, 20, 500);

            if (key && key->ok())
                keyW = std::max<double>(keyW, key->m_size.x);
            if (label && label->ok())
                labelW = std::max<double>(labelW, label->m_size.x);
            if (description && description->ok())
                descriptionW = std::max<double>(descriptionW, description->m_size.x);
        }

        const double contentW = 64.0 + keyW + 44.0 + labelW + 44.0 + descriptionW + 32.0;
        const double titleW   = title && title->ok() ? title->m_size.x + 48.0 : 0.0;
        const double searchW  = search && search->ok() ? search->m_size.x + 64.0 : 0.0;

        shortcuts.width      = std::min(viewSize.x - 64.0, std::max({900.0, contentW, titleW, searchW}));
        shortcuts.height     = std::min(viewSize.y - 64.0, 144.0 + (rowH * static_cast<double>(std::max<size_t>(1, actions.size()))));
        shortcuts.keyWidth   = keyW;
        shortcuts.labelWidth = labelW;
        shortcuts.descWidth  = descriptionW;
        shortcuts.sizeValid  = true;
    }

} // namespace hyprdeck
