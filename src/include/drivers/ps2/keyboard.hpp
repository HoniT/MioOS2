// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <drivers/ps2/ps2.hpp>
#include <util/singleton.hpp>
#include <util/spinlock.hpp>
#include <util/queue.hpp>

#define KEYBOARD_INT_VECTOR 33

// Hardware-independent list of keys
enum class KeyCode : uint32_t {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Enter, Escape, Backspace, Tab, Space,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    UpArrow, DownArrow, LeftArrow, RightArrow,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Minus, Equals, LeftBracket, RightBracket, Backslash,
    Semicolon, Apostrophe, Backtick, Comma, Period, Slash,
    CapsLock
};

// Bitmask for modifier keys
enum KeyModifiers : uint8_t {
    MOD_NONE  = 0,
    MOD_SHIFT = 1 << 0,
    MOD_CTRL  = 1 << 1,
    MOD_ALT   = 1 << 2,
    MOD_CAPS  = 1 << 3
};

struct key_event_t {
    KeyCode keycode;     // The exact key pressed (hardware independent)
    char ascii;          // The resolved ASCII character (0 if it's not printable)
    bool is_press;       // true if pressed, false if released
    uint8_t modifiers;   // State of Shift, Ctrl, Alt, CapsLock at the time of the event
};



class KeyboardDriver : public PS2DeviceDriver, public util::Singleton<KeyboardDriver> {
private:
    // State machine for multi-byte scancodes
    bool is_extended_scancode = false;
    bool is_release_sequence = false;
    
    // Modifier states
    bool shift_pressed = false;
    bool ctrl_pressed = false;
    bool alt_pressed = false;
    bool caps_lock_active = false;

    util::Queue<key_event_t> queue = util::Queue<key_event_t>();
    util::IrqSpinlock queue_lock;

    static const constexpr char lowercase_map[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\t', '`', 0,
        0, 0, 0, 0, 0, 'q', '1', 0, 0, 0, 'z', 's', 'a', 'w', '2', 0,
        0, 'c', 'x', 'd', 'e', '4', '3', 0, 0, ' ', 'v', 'f', 't', 'r', '5', 0,
        0, 'n', 'b', 'h', 'g', 'y', '6', 0, 0, 0, 'm', 'j', 'u', '7', '8', 0,
        0, ',', 'k', 'i', 'o', '0', '9', 0, 0, '.', '/', 'l', ';', 'p', '-', 0,
        0, 0, '\'', 0, '[', '=', 0, 0, 0, 0, '\n', ']', 0, '\\', 0, 0,
        0, 0, 0, 0, 0, 0, '\b', 0, 0, '1', 0, '4', '7', 0, 0, 0,
        '0', '.', '2', '5', '6', '8', 27, 0, 0, '+', '3', '-', '*', '9', 0, 0
    };

    static const constexpr char uppercase_map[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\t', '~', 0,
        0, 0, 0, 0, 0, 'Q', '!', 0, 0, 0, 'Z', 'S', 'A', 'W', '@', 0,
        0, 'C', 'X', 'D', 'E', '$', '#', 0, 0, ' ', 'V', 'F', 'T', 'R', '%', 0,
        0, 'N', 'B', 'H', 'G', 'Y', '^', 0, 0, 0, 'M', 'J', 'U', '&', '*', 0,
        0, '<', 'K', 'I', 'O', ')', '(', 0, 0, '>', '?', 'L', ':', 'P', '_', 0,
        0, 0, '\"', 0, '{', '+', 0, 0, 0, 0, '\n', '}', 0, '|', 0, 0,
        0, 0, 0, 0, 0, 0, '\b', 0, 0, '1', 0, '4', '7', 0, 0, 0,
        '0', '.', '2', '5', '6', '8', 27, 0, 0, '+', '3', '-', '*', '9', 0, 0
    };

public:
    KeyboardDriver() = default;
    KeyboardDriver(uint8_t port): PS2DeviceDriver(port) { }

    bool initialize() override;
    void start() override { }
    void stop() override { }

    void handle_interrupt(arch::interrupt_registers_t*) override;
    static inline void handle_interrupt_wrapper(arch::interrupt_registers_t* regs) {
        KeyboardDriver::get_instance()->handle_interrupt(regs);
    }

    key_event_t get_curr_key_event();
};

#endif // KEYBOARD_HPP
