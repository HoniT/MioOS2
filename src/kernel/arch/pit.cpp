// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Programmable Interval Timer
// ========================================

#include <arch/pit.hpp>
#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <kernel_ui.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>
#include <io.hpp>

using namespace arch;

uint32_t PIT::reload_count = 11931; // ~10ms interval (1193182Hz * 0.01s)

void PIT::wait_10ms() {
    // Enable PIT Channel 2 gate and disable the PC speaker
    uint8_t port_61_val = cpu::inb(0x61);
    cpu::outb(0x61, (port_61_val & 0xFD) | 0x01);

    cpu::outb(IO_PIT_CMD, 0xB0);

    cpu::outb(IO_PIT_CH2, (uint8_t)(reload_count & 0xFF));        // LSB
    cpu::outb(IO_PIT_CH2, (uint8_t)((reload_count >> 8) & 0xFF)); // MSB

    // Polling Port 0x61 bit 5 until the timer fires
    while ((cpu::inb(0x61) & 0x20) == 0) {
        asm volatile("pause");
    }

    // Disabling Channel 2 gate after we are done
    cpu::outb(0x61, port_61_val & 0xFC);
}
