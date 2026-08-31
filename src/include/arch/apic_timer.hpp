// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef APIC_TIMER_HPP
#define APIC_TIMER_HPP

#include <stdint.h>
#include <arch/interrupts/idt.hpp>

#define TIMER_INT_VECTOR 32

namespace arch
{
    class APICTimer {
    private:
        static void calibrate();
        static bool check_tsc_deadline_support();

        static bool has_tsc_deadline;
        static uint32_t apic_ticks_per_10ms;
        static uint64_t tsc_ticks_per_10ms;
        static uint64_t tsc_hz;
        static uint64_t next_tsc_deadline;
        
        // System time tracking
        static volatile uint64_t total_ticks;

    public:
        static void initialize();
        static void set_deadline_us(uint64_t microseconds);
        static void on_irq(interrupt_registers_t* regs);

        static uint64_t get_tsc_freq() { return tsc_hz; }
        static bool is_tsc_deadline_supported() { return has_tsc_deadline; }
    
        static uint64_t get_total_ticks() { return total_ticks; }
    };
} // namespace arch


#endif // APIC_TIMER_HPP
