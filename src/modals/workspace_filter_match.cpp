#include "workspace_filter_match.hpp"

#include "overlays.hpp"
#include "strings.hpp"
#include "plugin.hpp"
#include "workspace_filter.hpp"
#include "workspaces.hpp"

#include <desktop/view/window/Window.hpp>
#include <output/Monitor.hpp>
#include <workspace/HLWorkspace.hpp>

#include <algorithm>
#include <string_view>
#include <utility>

namespace hyprdeck {
    namespace {

        bool windowReadyForFilter(const PHLWINDOW& window) {
            if (!window || !window->mapped() || window->isHidden() || static_cast<bool>(window->m_state & Desktop::View::WINDOW_STATE_PINNED) ||
                activePlugin()->overlays().windowIsExternalOverlay(window))
                return false;

            return true;
        }

        bool windowTextMatchesFilter(const PHLWINDOW& window, std::string_view loweredQuery) {
            const auto& metadata = window->metadata();
            return strings::containsLowered(metadata.appID(), loweredQuery) || strings::containsLowered(metadata.initialAppID(), loweredQuery) ||
                strings::containsLowered(metadata.title(), loweredQuery) || strings::containsLowered(metadata.initialTitle(), loweredQuery);
        }

        bool workspaceNameMatchesFilter(const PHLWORKSPACE& workspace, std::string_view loweredQuery) {
            return workspace && (strings::containsLowered(workspace->displayName(), loweredQuery) || strings::containsLowered(workspace->addressableName(), loweredQuery));
        }

        bool workspaceMatchesFilter(const PHLWORKSPACE& workspace, std::string_view loweredQuery) {
            if (!workspace)
                return false;

            if (workspaceNameMatchesFilter(workspace, loweredQuery))
                return true;

            for (const auto& window : activePlugin()->hyprland().windows()) {
                if (!windowReadyForFilter(window) || !activePlugin()->workspaces().windowBelongsToWorkspace(window, workspace))
                    continue;

                if (windowTextMatchesFilter(window, loweredQuery))
                    return true;
            }

            return false;
        }

        std::vector<WORKSPACEID> normalWorkspaceIDsMatchingFilter(const std::vector<WORKSPACEID>& candidates, std::string_view loweredQuery) {
            std::vector<WORKSPACEID> ids;
            ids.reserve(candidates.size());

            for (const auto id : candidates) {
                const auto workspace = activePlugin()->workspaces().workspaceById(id);
                if (activePlugin()->workspaces().isNormalWorkspace(workspace) && workspaceMatchesFilter(workspace, loweredQuery))
                    ids.push_back(id);
            }

            return ids;
        }

    } // namespace

    SWorkspaceFilterRows CWorkspaceFilterMatcher::apply(const PHLMONITOR& monitor, std::vector<WORKSPACEID> normalWorkspaceIDs, std::vector<PHLWORKSPACE> specialWorkspaces) const {
        SWorkspaceFilterRows rows{.normalWorkspaceIDs = std::move(normalWorkspaceIDs), .specialWorkspaces = std::move(specialWorkspaces)};

        const auto           loweredQuery = strings::lower(activePlugin()->workspaceFilter().text());
        if (loweredQuery.empty())
            return rows;

        rows.normalWorkspaceIDs = normalWorkspaceIDsMatchingFilter(rows.normalWorkspaceIDs, loweredQuery);
        std::erase_if(rows.specialWorkspaces, [&](const auto& workspace) { return !workspaceMatchesFilter(workspace, loweredQuery); });
        return rows;
    }

} // namespace hyprdeck
