// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// APIC Timer
// ========================================

#include <arch/apic_timer.hpp>
#include <arch/interrupts/lapic.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>
#include <kernel_ui.hpp>
#include <arch/pit.hpp>

using namespace arch;

bool APICTimer::has_tsc_deadline = false;
uint32_t APICTimer::apic_ticks_per_10ms = 0;
uint64_t APICTimer::tsc_ticks_per_10ms = 0;
uint64_t APICTimer::tsc_hz = 0;
uint64_t APICTimer::next_tsc_deadline = 0;
volatile uint64_t APICTimer::total_ticks = 0;

bool APICTimer::check_tsc_deadline_support() {
    uint32_t eax = 1, ebx, ecx, edx;
    cpu::CPU::cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (ecx & (1 << 24)) != 0;
}

void APICTimer::calibrate() {
    // Prepare APIC Timer
    LAPIC::write_reg(0x3E0, 0x03); 
    PIT::prepare_10ms();

    LAPIC::write_reg(0x380, 0xFFFFFFFF);

    // Read starting TSC
    uint32_t lo_start, hi_start;
    asm volatile("rdtsc" : "=a"(lo_start), "=d"(hi_start));
    uint64_t start_tsc = ((uint64_t)hi_start << 32) | lo_start;

    // Wait exactly 10ms using the PIT
    PIT::poll_10ms();

    // Read ending TSC
    uint32_t lo_end, hi_end;
    asm volatile("rdtsc" : "=a"(lo_end), "=d"(hi_end));
    uint64_t end_tsc = ((uint64_t)hi_end << 32) | lo_end;

    // Read APIC count and stop it
    uint32_t current_apic = LAPIC::read_reg(0x390);
    LAPIC::write_reg(0x380, 0);

    // Calculate results
    apic_ticks_per_10ms = 0xFFFFFFFF - current_apic;
    tsc_ticks_per_10ms = end_tsc - start_tsc;
    tsc_hz = tsc_ticks_per_10ms * 100;

    kprintf(gui::LOG_INFO, "APIC Timer Calibrated: APIC=%d ticks/10ms, TSC=%u Hz\n", 
            apic_ticks_per_10ms, tsc_hz);
}

void APICTimer::initialize() {
    if(!LAPIC::initialized) return;

    calibrate();
    has_tsc_deadline = check_tsc_deadline_support();

    // Register the interrupt handler
    IDT::register_interrupt_handler(TIMER_INT_VECTOR, APICTimer::on_irq);

    if (has_tsc_deadline) {
        // TSC-Deadline mode: Set bit 17 in LVT Timer
        LAPIC::write_reg(0x320, TIMER_INT_VECTOR | (1 << 18));
        set_deadline_us(1000);

        kprintf(gui::LOG_INFO, "APIC Timer mode: TSC-Deadline\n");
    } else {
        // Periodic mode: Set bit 16 in LVT Timer
        LAPIC::write_reg(0x320, TIMER_INT_VECTOR | (1 << 17));
        LAPIC::write_reg(0x3E0, 0x03); 
        
        // Start a 1ms periodic tick
        LAPIC::write_reg(0x380, apic_ticks_per_10ms / 10);
        kprintf(gui::LOG_INFO, "APIC Timer mode: Periodic (1ms tick)\n");
    }
}

void APICTimer::set_deadline_us(uint64_t microseconds) {
    if (!has_tsc_deadline) return;

    // Calculate ticks for the requested microseconds
    uint64_t ticks_to_wait = (tsc_hz * microseconds) / 1000000;
    
    // If we haven't set a deadline yet, base it on the current time
    if (next_tsc_deadline == 0) {
        uint32_t lo, hi;
        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
        next_tsc_deadline = (((uint64_t)hi << 32) | lo) + ticks_to_wait;
    } else {
        // Otherwise, rigidly add to the previous deadline to prevent drift
        next_tsc_deadline += ticks_to_wait;
    }

    uint32_t msr_lo = next_tsc_deadline & 0xFFFFFFFF;
    uint32_t msr_hi = next_tsc_deadline >> 32;
    asm volatile("wrmsr" :: "c"(0x6E0), "a"(msr_lo), "d"(msr_hi));
}

void APICTimer::on_irq(interrupt_registers_t* regs) {
    total_ticks++;

    if (has_tsc_deadline) {
        set_deadline_us(1000);
    }
}
