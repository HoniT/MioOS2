// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// PS/2 keyboard driver
// ========================================

#include <drivers/ps2/keyboard.hpp>
#include <kernel_ui.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <arch/interrupts/lapic.hpp>
#include <cpu.hpp>

// Helper to map Set 2 physical scancodes to OS hardware-independent KeyCodes
static KeyCode map_set2_to_os_keycode(uint8_t key, bool is_extended) {
    if (is_extended) {
        switch (key) {
            case 0x75: return KeyCode::UpArrow;
            case 0x72: return KeyCode::DownArrow;
            case 0x6B: return KeyCode::LeftArrow;
            case 0x74: return KeyCode::RightArrow;
            case 0x14: return KeyCode::RightCtrl;
            case 0x11: return KeyCode::RightAlt;
            default:   return KeyCode::Unknown;
        }
    }

    switch (key) {
        case 0x1C: return KeyCode::A;
        case 0x32: return KeyCode::B;
        case 0x21: return KeyCode::C;
        case 0x23: return KeyCode::D;
        case 0x24: return KeyCode::E;
        case 0x2B: return KeyCode::F;
        case 0x34: return KeyCode::G;
        case 0x33: return KeyCode::H;
        case 0x43: return KeyCode::I;
        case 0x3B: return KeyCode::J;
        case 0x42: return KeyCode::K;
        case 0x4B: return KeyCode::L;
        case 0x3A: return KeyCode::M;
        case 0x31: return KeyCode::N;
        case 0x44: return KeyCode::O;
        case 0x4D: return KeyCode::P;
        case 0x15: return KeyCode::Q;
        case 0x2D: return KeyCode::R;
        case 0x1B: return KeyCode::S;
        case 0x2C: return KeyCode::T;
        case 0x3C: return KeyCode::U;
        case 0x2A: return KeyCode::V;
        case 0x1D: return KeyCode::W;
        case 0x22: return KeyCode::X;
        case 0x35: return KeyCode::Y;
        case 0x1A: return KeyCode::Z;

        case 0x16: return KeyCode::Num1;
        case 0x1E: return KeyCode::Num2;
        case 0x26: return KeyCode::Num3;
        case 0x25: return KeyCode::Num4;
        case 0x2E: return KeyCode::Num5;
        case 0x36: return KeyCode::Num6;
        case 0x3D: return KeyCode::Num7;
        case 0x3E: return KeyCode::Num8;
        case 0x46: return KeyCode::Num9;
        case 0x45: return KeyCode::Num0;

        case 0x0E: return KeyCode::Backtick;
        case 0x4E: return KeyCode::Minus;
        case 0x55: return KeyCode::Equals;
        case 0x5D: return KeyCode::Backslash;
        case 0x66: return KeyCode::Backspace;
        case 0x29: return KeyCode::Space;
        case 0x0D: return KeyCode::Tab;
        case 0x58: return KeyCode::CapsLock;
        case 0x12: return KeyCode::LeftShift;
        case 0x59: return KeyCode::RightShift;
        case 0x14: return KeyCode::LeftCtrl;
        case 0x11: return KeyCode::LeftAlt;
        case 0x5A: return KeyCode::Enter;
        case 0x76: return KeyCode::Escape;
        
        case 0x54: return KeyCode::LeftBracket;
        case 0x5B: return KeyCode::RightBracket;
        case 0x4C: return KeyCode::Semicolon;
        case 0x52: return KeyCode::Apostrophe;
        case 0x41: return KeyCode::Comma;
        case 0x49: return KeyCode::Period;
        case 0x4A: return KeyCode::Slash;

        default:   return KeyCode::Unknown;
    }
}

bool KeyboardDriver::initialize() {
    cpu::CPU::disable_interrupts();

    // Flush any garbage left over from the identification phase
    while(PS2Controller::wait_read(500)) { PS2Controller::read_data(); }

    // Disable scanning during configuration so incoming keypresses do not desync ACKs
    send_command_with_ack(0xF5);
    while(PS2Controller::wait_read(500)) { PS2Controller::read_data(); }

    // Set Scan Code Set to Set 2 (0xF0, then 0x02)
    if(!send_command_with_ack(0xF0)) {
        kprintf(gui::LOG_ERROR, "Keyboard failed to set to scancode set 2\n");
        cpu::CPU::enable_interrupts();
        return false;
    }
    if(!send_command_with_ack(0x02)) {
        kprintf(gui::LOG_ERROR, "Keyboard failed to set to scancode set 2\n");
        cpu::CPU::enable_interrupts();
        return false;
    }
    
    arch::register_interrupt_handler(KEYBOARD_INT_VECTOR, KeyboardDriver::handle_interrupt_wrapper);
    if (!arch::IOAPIC::find_gsi_and_write_rte(KEYBOARD_INT_VECTOR)) {
        kprintf(gui::LOG_ERROR, "Failed to resolve GSI and write RTE for Keyboard driver\n");
        cpu::CPU::enable_interrupts();
        return false;
    }

    if (!send_command_with_ack(0xF4)) {
        kprintf(gui::LOG_ERROR, "Keyboard failed to acknowledge 0xF4 (Enable Scanning)\n");
        cpu::CPU::enable_interrupts();
        return false;
    }

    // Final flush while interrupts are masked so IRQ 1 starts low for the next rising edge
    while(PS2Controller::wait_read(500)) { PS2Controller::read_data(); }

    kprintf(gui::LOG_INFO, "Initialized the PS/2 keyboard driver on PS/2 port %x!\n", port);
    cpu::CPU::enable_interrupts();
    return true;
}

void KeyboardDriver::handle_interrupt(arch::interrupt_registers_t* regs) {
    uint8_t scancode = PS2Controller::read_data();

    // Ignore PS/2 protocol control and buffer error bytes
    if (scancode == 0xFA || scancode == 0xFE || scancode == 0x00 || scancode == 0xFF) {
        return;
    }

    // Handle extended scancode
    if (scancode == 0xE0) {
        is_extended_scancode = true;
        return; // Wait for next interrupt
    }

    // Handle key release
    if (scancode == 0xF0) {
        is_release_sequence = true;
        return; // Wait for next interrupt to get the actual key
    }

    bool is_release = is_release_sequence;
    uint8_t key = scancode;

    if (!is_extended_scancode) {
        switch (key) {
            case 0x12: // Left Shift
            case 0x59: // Right Shift
                shift_pressed = !is_release;
                break;
            case 0x14: // Left Ctrl
                ctrl_pressed = !is_release;
                break;
            case 0x11: // Left Alt
                alt_pressed = !is_release;
                break;
            case 0x58: // Caps Lock
                if (!is_release) caps_lock_active = !caps_lock_active; 
                break;
        }
    } else {
        switch (key) {
            case 0x14: // Right Ctrl
                ctrl_pressed = !is_release;
                break;
            case 0x11: // Right Alt
                alt_pressed = !is_release;
                break;
        }
    }

    // Calculate modifier bitmask
    uint8_t current_modifiers = MOD_NONE;
    if (shift_pressed) current_modifiers |= MOD_SHIFT;
    if (ctrl_pressed)  current_modifiers |= MOD_CTRL;
    if (alt_pressed)   current_modifiers |= MOD_ALT;
    if (caps_lock_active) current_modifiers |= MOD_CAPS;

    // Decode to ASCII
    char ascii = 0;
    if (!is_release && !is_extended_scancode && key < 128) {
        bool use_upper = shift_pressed ^ caps_lock_active;
        ascii = use_upper ? uppercase_map[key] : lowercase_map[key];
    }

    // Push to queue
    KeyCode os_key = map_set2_to_os_keycode(key, is_extended_scancode);
    key_event_t event;
    event.keycode = os_key;
    event.ascii = ascii;
    event.is_press = !is_release;
    event.modifiers = current_modifiers;
    uint64_t rflags = queue_lock.lock();
    queue.push(event);
    queue_lock.unlock(rflags);

    // Reset state machine for the next keystroke sequence
    is_extended_scancode = false;
    is_release_sequence = false;
}

key_event_t KeyboardDriver::get_curr_key_event() {
    uint64_t rflags = queue_lock.lock();
    
    if (queue.empty()) {
        queue_lock.unlock(rflags);
        return {KeyCode::Unknown, 0, false, MOD_NONE};
    }

    key_event_t ev = queue.front();
    queue.pop();

    queue_lock.unlock(rflags);
    return ev;
}
