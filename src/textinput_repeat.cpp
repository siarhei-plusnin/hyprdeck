#include "textinput_repeat.hpp"

#include "keyboard.hpp"
#include "monitors.hpp"

#include <hyprutils/memory/UniquePtr.hpp>
#include <managers/eventLoop/EventLoopManager.hpp>
#include <managers/eventLoop/EventLoopTimer.hpp>

#include <algorithm>
#include <chrono>
#include <optional>

namespace hyprdeck {
    namespace {

        struct SRepeatState {
            SP<CEventLoopTimer>    timer;
            STextInputRepeatTarget target;
            ETextInputAction       action  = ETextInputAction::NONE;
            uint32_t               keycode = 0;
        };

        SRepeatState& repeatState() {
            static SRepeatState value;
            return value;
        }

        bool repeatTargetValid(const STextInputRepeatTarget& target) {
            return target.input && target.active && target.changed;
        }

        void ensureRepeatTimer() {
            auto& repeat = repeatState();
            if (repeat.timer || !g_pEventLoopManager)
                return;

            repeat.timer = makeShared<CEventLoopTimer>(
                std::nullopt,
                [](SP<CEventLoopTimer> self, void*) {
                    auto& repeat = repeatState();
                    if (repeat.action == ETextInputAction::NONE || !repeatTargetValid(repeat.target) || !repeat.target.active()) {
                        stopTextInputRepeat();
                        return;
                    }

                    const auto monitor = overviewMonitor();
                    if (!monitor) {
                        stopTextInputRepeat();
                        return;
                    }

                    auto* input = repeat.target.input();
                    if (!input) {
                        stopTextInputRepeat();
                        return;
                    }

                    if (input->apply(repeat.action))
                        repeat.target.changed(monitor);

                    const int rate = keyboardRepeatRate();
                    if (rate > 0)
                        self->updateTimeout(std::chrono::milliseconds(std::max(1, 1000 / rate)));
                },
                nullptr);

            g_pEventLoopManager->addTimer(repeat.timer);
        }

    } // namespace

    bool textInputActionRepeats(const ETextInputAction action) {
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

    void startTextInputRepeat(const ETextInputAction action, const uint32_t keycode, const STextInputRepeatTarget target) {
        if (!textInputActionRepeats(action) || !repeatTargetValid(target))
            return;

        const int rate = keyboardRepeatRate();
        if (rate <= 0)
            return;

        ensureRepeatTimer();
        auto& repeat   = repeatState();
        repeat.target  = target;
        repeat.action  = action;
        repeat.keycode = keycode;

        if (repeat.timer)
            repeat.timer->updateTimeout(std::chrono::milliseconds(std::max(1, keyboardRepeatDelay())));
    }

    void stopTextInputRepeat() {
        auto& repeat   = repeatState();
        repeat.target  = {};
        repeat.action  = ETextInputAction::NONE;
        repeat.keycode = 0;

        if (repeat.timer)
            repeat.timer->updateTimeout(std::nullopt);
    }

    void stopTextInputRepeatFor(const uint32_t keycode) {
        if (repeatState().keycode == keycode)
            stopTextInputRepeat();
    }

    void resetTextInputRepeatComponent() {
        stopTextInputRepeat();

        auto& repeat = repeatState();
        if (repeat.timer && g_pEventLoopManager)
            g_pEventLoopManager->removeTimer(repeat.timer);

        repeat.timer.reset();
    }

} // namespace hyprdeck
