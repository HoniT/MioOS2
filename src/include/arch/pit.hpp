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
#define IO_PIT_CH2 0x42

namespace arch
{
    class PIT {
    private:
        static uint32_t reload_count;

    public:
        static void wait_10ms();
    };
} // namespace arch


#endif // PIT_HPP
