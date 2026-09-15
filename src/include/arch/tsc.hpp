// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TSC_HPP
#define TSC_HPP

#include <stdint.h>

namespace arch
{
    class TSC {
    private:
        static uint64_t tsc_hz;
        static bool calibrated;

    public:
        static void calibrate();
        static uint64_t rdtsc();
        static void delay_us(uint64_t microseconds);
        static uint64_t get_ns();

        static inline uint64_t get_tsc_hz() { return tsc_hz; }
    };
} // namespace arch

#endif // TSC_HPP
