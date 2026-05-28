#include "naming.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "plugin.hpp"
#include "runtime_types.hpp"
#include "textinput.hpp"
#include "textinput_render.hpp"

#include <helpers/Monitor.hpp>
#include <render/Texture.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace hyprdeck {
    namespace {

        SP<Render::ITexture> labelTexture(const std::string& label, const int fontSize = 26, const int weight = 700, const ETextCacheMode cacheMode = ETextCacheMode::PERSISTENT) {
            return activePlugin()->renderServices().textTexture("naming", label, colors::textPrimary(), fontSize, weight, cacheMode);
        }

        void renderComponentBox(const CBox& box) {
            activePlugin()->renderServices().addRect(activePlugin()->renderServices().expandedBox(box, 2.0), colors::componentBorder());
            activePlugin()->renderServices().addRect(box, colors::componentBackground());
        }

    } // namespace

    void CNamingController::render(const PHLMONITOR& monitor) const {
        const auto& naming = m_state;
        if (naming.promptMode == EPromptMode::NONE)
            return;

        const bool createPrompt = naming.promptMode == EPromptMode::CREATE_SPECIAL;
        const auto names        = createPrompt ? activePlugin()->config().specialWorkspaceNames() : std::vector<std::string>{};

        const auto   viewSize       = monitor->m_transformedSize;
        const double boxW           = std::min(620.0, viewSize.x * 0.70);
        const double rowH           = 44.0;
        const bool   showInput      = !createPrompt || !naming.promptInput.text.empty();
        const double inputH         = showInput ? 48.0 : 0.0;
        const double maxBoxH        = std::max(124.0 + inputH, viewSize.y - 64.0);
        const double availableRowsH = std::max(0.0, maxBoxH - 80.0 - inputH);
        const size_t maxVisibleRows = names.empty() ? 0 : std::max<size_t>(1, static_cast<size_t>(availableRowsH / rowH));
        const size_t visibleRows    = std::min(names.size(), maxVisibleRows);
        const double boxH           = 80.0 + inputH + (rowH * static_cast<double>(visibleRows));
        const CBox   box{(viewSize.x - boxW) / 2.0, (viewSize.y - boxH) / 2.0, boxW, boxH};

        renderComponentBox(box);

        const auto title = labelTexture(createPrompt ? "Create special workspace" : "Rename special workspace", 22, 750);
        if (title && title->ok())
            activePlugin()->renderServices().addTexture(title, CBox{box.x + 24.0, box.y + 22.0, title->m_size.x, title->m_size.y});

        if (showInput) {
            const CBox inputBox{box.x + 18.0, box.y + 64.0, box.w - 36.0, 42.0};
            activePlugin()->renderServices().addRect(inputBox, colors::componentSurface());

            renderTextInputLine("naming", naming.promptInput, inputBox, "", colors::textPrimary(), 20, 500);
        }

        if (names.empty())
            return;

        const size_t selected = std::min(naming.namedSpecialPromptIndex, names.size() - 1);
        size_t       first    = 0;
        if (visibleRows < names.size()) {
            const size_t half = visibleRows / 2;
            first             = selected > half ? selected - half : 0;
            first             = std::min(first, names.size() - visibleRows);
        }

        for (size_t rowIndex = 0; rowIndex < visibleRows; ++rowIndex) {
            const size_t i   = first + rowIndex;
            const CBox   row{box.x + 18.0, box.y + 64.0 + inputH + (static_cast<double>(rowIndex) * rowH), box.w - 36.0, rowH - 8.0};
            if (!naming.promptCustomSelected && i == selected)
                activePlugin()->renderServices().addRect(row, colors::componentSelected());

            const auto texture = labelTexture(names[i], 20, !naming.promptCustomSelected && i == selected ? 700 : 500);
            if (texture && texture->ok())
                activePlugin()->renderServices().addTexture(texture, CBox{row.x + 14.0, row.y + ((row.h - texture->m_size.y) / 2.0), texture->m_size.x, texture->m_size.y});
        }
    }

} // namespace hyprdeck
