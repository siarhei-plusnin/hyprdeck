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

        if (current.filter.promptOpen)
            return EInputMode::FILTER;

        if (current.confirmation.open)
            return EInputMode::CONFIRMATION;

        return EInputMode::OVERVIEW;
    }

} // namespace hyprdeck
