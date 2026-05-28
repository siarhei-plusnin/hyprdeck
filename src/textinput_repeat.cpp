#include "textinput_repeat.hpp"

#include "keyboard.hpp"
#include "plugin.hpp"

#include <hyprutils/memory/UniquePtr.hpp>
#include <managers/eventLoop/EventLoopTimer.hpp>

#include <algorithm>
#include <chrono>
#include <optional>

namespace hyprdeck {
    namespace {

        bool repeatTargetValid(const STextInputRepeatTarget& target) {
            return target.input && target.active && target.changed;
        }

    } // namespace

    bool CTextInputRepeater::actionRepeats(const ETextInputAction action) const {
        switch (action) {
            case ETextInputAction::DELETE_BACKWARD:
            case ETextInputAction::DELETE_FORWARD:
            case ETextInputAction::DELETE_WORD_BACKWARD:
            case ETextInputAction::DELETE_WORD_FORWARD:
            case ETextInputAction::MOVE_LEFT:
            case ETextInputAction::MOVE_RIGHT:
            case ETextInputAction::MOVE_WORD_LEFT:
            case ETextInputAction::MOVE_WORD_RIGHT: return true;
            case ETextInputAction::NONE:
            case ETextInputAction::MOVE_START:
            case ETextInputAction::MOVE_END:
            case ETextInputAction::CLEAR:
            case ETextInputAction::CLEAR_TO_END: return false;
        }

        return false;
    }

    void CTextInputRepeater::ensureTimer() {
        if (m_timer)
            return;

        m_timer = makeShared<CEventLoopTimer>(
            std::nullopt,
            [](SP<CEventLoopTimer> self, void* data) {
                if (auto* repeater = static_cast<CTextInputRepeater*>(data); repeater)
                    repeater->handleTimer(self);
            },
            this);

        activePlugin()->hyprland().addTimer(m_timer);
    }

    void CTextInputRepeater::handleTimer(SP<CEventLoopTimer> self) {
        if (m_action == ETextInputAction::NONE || !repeatTargetValid(m_target) || !m_target.active()) {
            stop();
            return;
        }

        const auto monitor = activePlugin()->overview().monitor();
        if (!monitor) {
            stop();
            return;
        }

        auto* input = m_target.input();
        if (!input) {
            stop();
            return;
        }

        if (input->apply(m_action))
            m_target.changed(monitor);

        const int rate = activePlugin()->hyprland().keyboardRepeatRate();
        if (rate > 0)
            self->updateTimeout(std::chrono::milliseconds(std::max(1, 1000 / rate)));
    }

    void CTextInputRepeater::start(const ETextInputAction action, const uint32_t keycode, const STextInputRepeatTarget target) {
        if (!actionRepeats(action) || !repeatTargetValid(target))
            return;

        const int rate = activePlugin()->hyprland().keyboardRepeatRate();
        if (rate <= 0)
            return;

        ensureTimer();
        m_target  = target;
        m_action  = action;
        m_keycode = keycode;

        if (m_timer)
            m_timer->updateTimeout(std::chrono::milliseconds(std::max(1, activePlugin()->hyprland().keyboardRepeatDelay())));
    }

    void CTextInputRepeater::stop() {
        m_target  = {};
        m_action  = ETextInputAction::NONE;
        m_keycode = 0;

        if (m_timer)
            m_timer->updateTimeout(std::nullopt);
    }

    void CTextInputRepeater::stopFor(const uint32_t keycode) {
        if (m_keycode == keycode)
            stop();
    }

    void CTextInputRepeater::reset() {
        stop();

        activePlugin()->hyprland().removeTimer(m_timer);

        m_timer.reset();
    }

} // namespace hyprdeck
