// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Programmable Interval Timer
// ========================================

#include <arch/timers/pit.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <kernel_ui.hpp>
#include <cpu.hpp>
#include <io.hpp>
#include <timekeeping.hpp>

using namespace arch;

volatile uint64_t PIT::ticks = 0;

void PIT::tick_handler(interrupt_registers_t* regs) {
    ticks++;
}


void PIT::initialize() {
    // LSB/MSB, Mode 2 (Rate Generator), Binary for Channel 0
    cpu::outb(IO_PIT_CMD, 0x34); 
    
    // Write the divider count to Channel 0
    cpu::outb(IO_PIT_CH0, (uint8_t)(DEFAULT_DIVIDER & 0xFF));
    cpu::outb(IO_PIT_CH0, (uint8_t)((DEFAULT_DIVIDER >> 8) & 0xFF));
    
    arch::register_interrupt_handler(PIT_VECTOR, tick_handler);
    cpu::CPU::enable_interrupts();

    KernelTime::initialize(DEFAULT_FREQ, get_ns, delay_us, "_PIT");

    // Warmup
    arch::PIT::prepare_10ms();
    arch::PIT::poll_10ms();
    arch::PIT::prepare_10ms();
    arch::PIT::poll_10ms();

    initialized = true;
    kprintf(gui::LOG_INFO, "Initialized PIT as the main timer!\n");
}

void PIT::write_rte_for_pit() {
    if(!IOAPIC::find_gsi_and_write_rte(PIT_VECTOR)) {
        kprintf(gui::LOG_ERROR, "Couldn't write RTE for PIT!\n");
        demote();
    }
    kprintf(gui::LOG_INFO, "Wrote RTE for PIT!\n");
}

uint64_t PIT::get_ns() {
    return (ticks * 1000000000ULL) / DEFAULT_FREQ;
}

void PIT::delay_us(uint64_t us) {
    if(!initialized) return;

    uint64_t target_ticks = ticks + ((us * DEFAULT_FREQ) / 1000000ULL);
    
    while (ticks < target_ticks) {
        asm volatile("pause");
    }
}


void PIT::demote() {
    if(!initialized) return;
    initialized = false;

    arch::unregister_interrupt_handler(PIT_VECTOR);
}


void PIT::prepare_10ms() {
    uint8_t port_61_val = cpu::inb(0x61);
    cpu::outb(0x61, port_61_val & 0xFC);

    cpu::outb(IO_PIT_CMD, 0xB0);

    uint16_t count_10ms = 11931; 
    cpu::outb(IO_PIT_CH2, (uint8_t)(count_10ms & 0xFF));
    cpu::outb(IO_PIT_CH2, (uint8_t)((count_10ms >> 8) & 0xFF));

    port_61_val = cpu::inb(0x61);
    cpu::outb(0x61, port_61_val | 0x01);
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
