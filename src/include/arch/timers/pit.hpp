// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PIT_HPP
#define PIT_HPP

#include <stdint.h>
#include <arch/interrupts/interrupts.hpp>

#define PIT_VECTOR 32

#define IO_PIT_CH0 0x40
#define IO_PIT_CH2 0x42
#define IO_PIT_CMD 0x43

#define DEFAULT_FREQ 1000
#define DEFAULT_DIVIDER 1193182 / DEFAULT_FREQ

namespace arch
{
    /// @brief Legacy 8254 PIT
    class PIT {
    private:
        static volatile uint64_t ticks;
    
        static void tick_handler(interrupt_registers_t* regs);
    
    public:
        static inline bool initialized = false;
        /// @brief Initializes the PIT as the main timer source to Channel 0, IRQ 0. This should only be for early boot and 
        ///        it should be demoted to just calibrating other timers as soon as we find a more precise timer (TSC, HPET) 
        static void initialize();

        /// @brief If the PIT isn't disabled by the time we initialize I/OAPIC
        ///        (mask out the PIC and switch the IMCR mode) we need to write a RTE for the PIT in the IOAPIC
        static void write_rte_for_pit();

        static uint64_t get_ns();
        static void delay_us(uint64_t us);
        
        /// @brief Demoted PIT from main timer to calibration timer 
        static void demote();
        
        // === Calibration Methods ===

        static void prepare_10ms();
        static void poll_10ms();
    };
} // namespace arch


#endif // PIT_HPP
