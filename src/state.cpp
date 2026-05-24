#include "state.hpp"

namespace hyprdeck {

    SOverviewState& state() {
        static SOverviewState value;
        return value;
    }

    EInputMode currentInputMode() {
        const auto& current = state();
        if (!current.session.active)
            return EInputMode::INACTIVE;

        if (current.shortcuts.open)
            return EInputMode::SHORTCUTS;

        if (current.naming.promptMode != EPromptMode::NONE)
            return EInputMode::NAMING;

        return EInputMode::OVERVIEW;
    }

} // namespace hyprdeck
