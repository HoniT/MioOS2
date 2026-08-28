// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PIT_HPP
#define PIT_HPP

#include <stdint.h>
#include <arch/interrupts/idt.hpp>

#define PIT_VECTOR 32

#define IO_PIT_CMD 0x43
#define IO_PIT_CH0 0x40

#define PIT_FREQUENCY 1193182

namespace arch
{
    class PIT {
    private:
        static bool initialized;
        
    public:
        static volatile uint64_t ticks;
        static void initialize();
        
        static void on_irq(arch::interrupt_registers_t* regs);
    };
} // namespace arch


#endif // PIT_HPP
