// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PIT_HPP
#define PIT_HPP

#include <stdint.h>

#define IO_PIT_CMD 0x43
#define IO_PIT_CH2 0x42

namespace arch
{
    class PIT {
    private:
        static uint32_t reload_count;

    public:
        static void prepare_10ms();
        static void poll_10ms();
    };
} // namespace arch


#endif // PIT_HPP
