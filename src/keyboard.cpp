#include "keyboard.hpp"

#include <managers/SeatManager.hpp>
#include <managers/input/InputManager.hpp>

namespace hyprdeck {

    SKeyboardModifiers keyboardModifiers() {
        SKeyboardModifiers modifiers;

        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard) {
            const auto activeModifiers = keyboard->getModifiers();
            modifiers.ctrl             = modifiers.ctrl || (activeModifiers & HL_MODIFIER_CTRL);
            modifiers.shift            = modifiers.shift || (activeModifiers & HL_MODIFIER_SHIFT);
        }

        for (const auto& keyboard : g_pInputManager->m_keyboards) {
            if (!keyboard)
                continue;

            const auto activeModifiers = keyboard->getModifiers();
            modifiers.ctrl             = modifiers.ctrl || (activeModifiers & HL_MODIFIER_CTRL);
            modifiers.shift            = modifiers.shift || (activeModifiers & HL_MODIFIER_SHIFT);
        }

        return modifiers;
    }

    bool ctrlPressed() {
        return keyboardModifiers().ctrl;
    }

    bool shiftPressed() {
        return keyboardModifiers().shift;
    }

    int keyboardRepeatRate() {
        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard)
            return keyboard->m_repeatRate;

        return 25;
    }

    int keyboardRepeatDelay() {
        if (const auto keyboard = g_pSeatManager ? g_pSeatManager->m_keyboard.lock() : nullptr; keyboard)
            return keyboard->m_repeatDelay;

        return 600;
    }

} // namespace hyprdeck
