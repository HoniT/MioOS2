// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Programmable Interval Timer
// ========================================

#include <arch/pit.hpp>
#include <arch/interrupts/lapic.hpp>
#include <cpu.hpp>
#include <io.hpp>

using namespace arch;

uint32_t PIT::reload_count = 11931; // ~10ms interval (1193182Hz * 0.01s)

void PIT::prepare_10ms() {
    // Enable PIT Channel 2 gate and disable the PC speaker
    uint8_t port_61_val = cpu::inb(0x61);
    cpu::outb(0x61, (port_61_val & 0xFD) | 0x01);

    // LSB/MSB, Mode 0 (One-shot), Binary
    cpu::outb(IO_PIT_CMD, 0xB0);

    // Write the reload count
    cpu::outb(IO_PIT_CH2, (uint8_t)(reload_count & 0xFF));
    cpu::outb(IO_PIT_CH2, (uint8_t)((reload_count >> 8) & 0xFF));
}

void PIT::poll_10ms() {
    // Polling Port 0x61 bit 5 until the timer fires
    while ((cpu::inb(0x61) & 0x20) == 0) {
        asm volatile("pause");
    }

    // Disable Channel 2 gate after we are done
    uint8_t port_61_val = cpu::inb(0x61);
    cpu::outb(0x61, port_61_val & 0xFC);
}
