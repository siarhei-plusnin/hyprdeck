#pragma once

#include "textinput.hpp"

#include <desktop/DesktopTypes.hpp>

#include <cstdint>

class CEventLoopTimer;

namespace hyprdeck {

    using FTextInputProvider = STextInputState* (*)();
    using FTextInputActive   = bool (*)();
    using FTextInputChanged  = void (*)(const PHLMONITOR& monitor);

    struct STextInputRepeatTarget {
        FTextInputProvider input   = nullptr;
        FTextInputActive   active  = nullptr;
        FTextInputChanged  changed = nullptr;
    };

    class CTextInputRepeater {
      public:
        bool actionRepeats(ETextInputAction action) const;
        void start(ETextInputAction action, uint32_t keycode, STextInputRepeatTarget target);
        void stop();
        void stopFor(uint32_t keycode);
        void reset();

      private:
        void ensureTimer();
        void handleTimer(SP<CEventLoopTimer> self);

        SP<CEventLoopTimer>    m_timer;
        STextInputRepeatTarget m_target;
        ETextInputAction       m_action  = ETextInputAction::NONE;
        uint32_t               m_keycode = 0;
    };

} // namespace hyprdeck
