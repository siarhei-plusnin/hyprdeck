#pragma once

#include "textinput.hpp"

#include <desktop/DesktopTypes.hpp>

#include <cstdint>

namespace hyprdeck {

    using FTextInputProvider = STextInputState* (*)();
    using FTextInputActive   = bool (*)();
    using FTextInputChanged  = void (*)(const PHLMONITOR& monitor);

    struct STextInputRepeatTarget {
        FTextInputProvider input   = nullptr;
        FTextInputActive   active  = nullptr;
        FTextInputChanged  changed = nullptr;
    };

    bool textInputActionRepeats(ETextInputAction action);
    void startTextInputRepeat(ETextInputAction action, uint32_t keycode, STextInputRepeatTarget target);
    void stopTextInputRepeat();
    void stopTextInputRepeatFor(uint32_t keycode);
    void resetTextInputRepeatComponent();

} // namespace hyprdeck
